// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "trader/oms.h"

#include <dlfcn.h>

#include <utility>

#include "ft/base/contract_table.h"
#include "ft/component/pubsub/subscriber.h"
#include "ft/utils/config_loader.h"
#include "ft/utils/lockfree-queue/queue.h"
#include "ft/utils/misc.h"
#include "ft/utils/protocol_utils.h"
#include "spdlog/spdlog.h"
#include "trader/risk/common/fund_manager.h"
#include "trader/risk/common/no_self_trade.h"
#include "trader/risk/common/position_manager.h"
#include "trader/risk/common/strategy_notifier.h"
#include "trader/risk/common/throttle_rate_limit.h"

namespace ft {

namespace {
void ShowConfig(const Config& cfg) {
  spdlog::info("Config:");
  spdlog::info("  api: {}", cfg.api);
  spdlog::info("  trade_server_address: {}", cfg.trade_server_address);
  spdlog::info("  quote_server_address: {}", cfg.quote_server_address);
  spdlog::info("  broker_id: {}", cfg.broker_id);
  spdlog::info("  investor_id: {}", cfg.investor_id);
  spdlog::info("  password: ******");
  spdlog::info("  auth_code: {}", cfg.auth_code);
  spdlog::info("  app_id: {}", cfg.app_id);
  spdlog::info("  subcription_list: {}", fmt::join(cfg.subscription_list, ","));
  spdlog::info("  contracts_file: {}", cfg.contracts_file.c_str());
  spdlog::info("  cancel_outstanding_orders_on_startup: {}",
               cfg.cancel_outstanding_orders_on_startup);
  if (!cfg.arg0.empty()) spdlog::info("  arg0: {}", cfg.arg0);
  if (!cfg.arg1.empty()) spdlog::info("  arg1: {}", cfg.arg1);
  if (!cfg.arg2.empty()) spdlog::info("  arg2: {}", cfg.arg2);
  if (!cfg.arg3.empty()) spdlog::info("  arg3: {}", cfg.arg3);
  if (!cfg.arg4.empty()) spdlog::info("  arg4: {}", cfg.arg4);
  if (!cfg.arg5.empty()) spdlog::info("  arg5: {}", cfg.arg5);
  if (!cfg.arg6.empty()) spdlog::info("  arg6: {}", cfg.arg6);
  if (!cfg.arg7.empty()) spdlog::info("  arg7: {}", cfg.arg7);
  if (!cfg.arg8.empty()) spdlog::info("  arg8: {}", cfg.arg8);
}
}  // namespace

OrderManagementSystem::OrderManagementSystem() : md_pusher_("ipc://md.ft_trader.ipc") {
  rms_ = std::make_unique<RiskManagementSystem>();
}

bool OrderManagementSystem::Init(const Config& config) {
  spdlog::info("compiling time: {} {}", __TIME__, __DATE__);
  ShowConfig(config);

  // 如果有配置contracts file的路径，则尝试从文件加载
  if (!config.contracts_file.empty()) {
    if (!ContractTable::Init(config.contracts_file))
      spdlog::warn("failed to load contract table from file {}，try to query from gateway later",
                   config.contracts_file);
  }

  cmd_queue_key_ = config.key_of_cmd_queue;

  gateway_ = LoadGateway(config.api);
  if (!gateway_) {
    return false;
  }

  if (!gateway_->Init(config)) {
    spdlog::error("failed to init gateway");
    return false;
  }
  spdlog::info("gateway inited. account: {}", config.investor_id);

  auto qry_res_rb = gateway_->GetQryResultRB();
  GatewayQueryResult qry_res;

  if (!ContractTable::is_inited()) {
    std::vector<Contract> contracts;
    if (!gateway_->QueryContracts()) {
      spdlog::error("failed to query contracts.");
      return false;
    }
    for (;;) {
      qry_res_rb->GetWithBlocking(&qry_res);
      if (qry_res.msg_type == GatewayMsgType::kContractEnd) {
        break;
      }
      assert(qry_res.msg_type == GatewayMsgType::kContract);
      contracts.emplace_back(std::get<Contract>(qry_res.data));
    }
    ContractTable::Init(std::move(contracts));
    ContractTable::Store("./contracts.csv");
  }

  if (!gateway_->Subscribe(config.subscription_list)) {
    spdlog::error("failed to subscribe market data");
    return false;
  }

  if (!gateway_->QueryAccount()) {
    spdlog::error("failed to query account");
    return false;
  }
  qry_res_rb->GetWithBlocking(&qry_res);
  if (qry_res.msg_type != GatewayMsgType::kAccount) {
    spdlog::error("failed to query account");
    return false;
  }
  OnAccount(std::get<Account>(qry_res.data));
  qry_res_rb->GetWithBlocking(&qry_res);
  assert(qry_res.msg_type == GatewayMsgType::kAccountEnd);

  // query all positions
  redis_pos_updater_.SetAccount(account_.account_id);
  redis_pos_updater_.Clear();
  pos_calculator_.SetCallback([this](const Position& new_pos) {
    auto* contract = ContractTable::get_by_index(new_pos.ticker_id);
    if (!contract) {
      spdlog::error("contract not found. failed to update positions in redis. ticker_id:{}",
                    new_pos.ticker_id);
      return;
    }
    redis_pos_updater_.set(contract->ticker, new_pos);
  });

  std::vector<Position> init_positions;
  if (!gateway_->QueryPositions()) {
    spdlog::error("failed to query positions");
    return false;
  }
  for (;;) {
    qry_res_rb->GetWithBlocking(&qry_res);
    if (qry_res.msg_type == GatewayMsgType::kPositionEnd) {
      break;
    }
    assert(qry_res.msg_type == GatewayMsgType::kPosition);
    init_positions.emplace_back(std::get<Position>(qry_res.data));
  }
  OnPositions(&init_positions);

  // query trades to update position
  std::vector<Trade> init_trades;
  if (!gateway_->QueryTrades()) {
    spdlog::error("failed to query trades");
    return false;
  }
  for (;;) {
    qry_res_rb->GetWithBlocking(&qry_res);
    if (qry_res.msg_type == GatewayMsgType::kTradeEnd) {
      break;
    }
    assert(qry_res.msg_type == GatewayMsgType::kTrade);
    init_trades.emplace_back(std::get<Trade>(qry_res.data));
  }
  OnTrades(&init_trades);

  // Init risk manager
  RiskRuleParams risk_params{};
  risk_params.config = &config;
  risk_params.account = &account_;
  risk_params.pos_calculator = &pos_calculator_;
  risk_params.order_map = &order_map_;
  rms_->AddRule(std::make_shared<FundManager>());
  rms_->AddRule(std::make_shared<PositionManager>());
  rms_->AddRule(std::make_shared<NoSelfTradeRule>());
  rms_->AddRule(std::make_shared<ThrottleRateLimit>());
  if (!config.no_receipt_mode) rms_->AddRule(std::make_shared<StrategyNotifier>());
  if (!rms_->Init(&risk_params)) {
    spdlog::error("failed to init rms");
    return false;
  }

  // 启动个线程去定时查询资金账户信息
  timer_thread_.AddTask(15 * 1000, std::mem_fn(&OrderManagementSystem::OnTimer), this);
  timer_thread_.Start();

  spdlog::info("ft_trader inited");
  is_logon_ = true;

  std::thread([this] {
    auto* rsp_rb = gateway_->GetOrderRspRB();
    GatewayOrderResponse rsp;
    for (;;) {
      rsp_rb->GetWithBlocking(&rsp);
      std::visit(*this, rsp.data);
    }
  }).detach();

  std::thread([this] {
    auto* tick_rb = gateway_->GetTickRB();
    TickData tick;
    for (;;) {
      tick_rb->GetWithBlocking(&tick);
      OnTick(tick);
    }
  }).detach();

  return true;
}

void OrderManagementSystem::ProcessCmd() {
  if (cmd_queue_key_ > 0)
    ProcessQueueCmd();
  else
    ProcessPubSubCmd();
}

void OrderManagementSystem::ProcessPubSubCmd() {
  pubsub::Subscriber cmd_sub("ipc://trade.ft_trader.ipc");

  auto ft_cmd_topic = std::string("ft_cmd_") + std::to_string(account_.account_id).substr(0, 4);
  if (!cmd_sub.Subscribe<TraderCommand>(ft_cmd_topic,
                                        [this](TraderCommand* cmd) { ExecuteCmd(*cmd); })) {
    spdlog::error("cannot subscribe order request");
    exit(EXIT_FAILURE);
  }

  spdlog::info("ft_trader start to retrieve orders from ipc://trade.ft_trader.ipc, topic:{}",
               ft_cmd_topic);
  cmd_sub.Start();
}

void OrderManagementSystem::ProcessQueueCmd() {
  // 创建Queue的时候存在隐患，如果该key之前被其他应用创建且没有释放，
  // OMS还是会依附在这个key的共享内存上，大概率导致内存访问越界。
  // 如果都是同版本的OMS创建的则没有问题，可以重复使用。
  // 释放队列暂时需要手动释放，请使用ipcrm。
  // TODO(kevin): 在LFQueue_create和LFQueue_open的时候加上检验信息。 Done!

  // user_id用于验证是否是trading engine创建的queue
  uint32_t te_user_id = 88888;
  LFQueue* cmd_queue;
  if ((cmd_queue = LFQueue_open(cmd_queue_key_, te_user_id)) == nullptr) {
    int res = LFQueue_create(cmd_queue_key_, te_user_id, sizeof(TraderCommand), 4096 * 4, false);
    if (res != 0) {
      spdlog::info("failed to create order queue. please ensure the shm key is free: {:#x}",
                   cmd_queue_key_);
      abort();
    }

    if ((cmd_queue = LFQueue_open(cmd_queue_key_, te_user_id)) == nullptr) {
      spdlog::info("failed to open the order queue. please ensure the shm key is free: {:#x}",
                   cmd_queue_key_);
      abort();
    }
  }

  LFQueue_reset(cmd_queue);
  spdlog::info("start to retrieve orders from order queue. key: {:#x}", cmd_queue_key_);

  TraderCommand cmd{};
  int res;
  for (;;) {
    // 这里没有使用零拷贝的方式，性能影响甚微
    res = LFQueue_pop(cmd_queue, &cmd, nullptr, nullptr);
    if (res != 0) continue;

    ExecuteCmd(cmd);
  }
}

void OrderManagementSystem::ExecuteCmd(const TraderCommand& cmd) {
#ifdef FT_TIME_PERF
  auto start_time = std::chrono::steady_clock::now();
#endif

  if (cmd.magic != kTradingCmdMagic) {
    spdlog::error("invalid cmd: invalid magic number");
    return;
  }

  switch (cmd.type) {
    case CMD_NEW_ORDER: {
      SendOrder(cmd);
      break;
    }
    case CMD_CANCEL_ORDER: {
      CancelOrder(cmd.cancel_req.order_id);
      break;
    }
    case CMD_CANCEL_TICKER: {
      CancelForTicker(cmd.cancel_ticker_req.ticker_id);
      break;
    }
    case CMD_CANCEL_ALL: {
      CancelAll();
      break;
    }
    case CMD_NOTIFY: {
      gateway_->OnNotify(cmd.notification.signal);
      break;
    }
    default: {
      spdlog::error("unknown cmd");
      break;
    }
  }

#ifdef FT_TIME_PERF
  auto end_time = std::chrono::steady_clock::now();
  spdlog::info("[OMS::ExecuteCmd] cmd type: {}   time elapsed: {} ns", cmd.type,
               (end_time - start_time).count());
#endif
}

bool OrderManagementSystem::SendOrder(const TraderCommand& cmd) {
  auto contract = ContractTable::get_by_index(cmd.order_req.ticker_id);
  if (!contract) {
    spdlog::error("[OMS::SendOrder] contract not found");
    return false;
  }

  Order order{};
  auto& req = order.req;
  req.order_id = next_order_id();
  req.contract = contract;
  req.direction = cmd.order_req.direction;
  req.offset = cmd.order_req.offset;
  req.volume = cmd.order_req.volume;
  req.type = cmd.order_req.type;
  req.price = cmd.order_req.price;
  req.flags = cmd.order_req.flags;
  order.client_order_id = cmd.order_req.client_order_id;
  order.status = OrderStatus::SUBMITTING;
  order.strategy_id = cmd.strategy_id;

  std::unique_lock<SpinLock> lock(spinlock_);
  // 增加是否经过风控检查字段，在紧急情况下可以设置该字段绕过风控下单
  if (!cmd.order_req.without_check) {
    int error_code = rms_->CheckOrderRequest(&order);
    if (error_code != NO_ERROR) {
      spdlog::error("[OMS::SendOrder] risk: {}", ErrorCodeStr(error_code));
      rms_->OnOrderRejected(&order, error_code);
      return false;
    }
  }

  if (!gateway_->SendOrder(req, &order.privdata)) {
    spdlog::error("[OMS::SendOrder] failed to send order. {}, {}{}, {}, Volume:{}, Price:{:.3f}",
                  contract->ticker, ToString(req.direction), ToString(req.offset),
                  ToString(req.type), req.volume, req.price);

    rms_->OnOrderRejected(&order, ERR_SEND_FAILED);
    return false;
  }

  order_map_.emplace(req.order_id, order);
  rms_->OnOrderSent(&order);

  spdlog::debug("[OMS::SendOrder] success. {}, {}{}, {}, OrderID:{}, Volume:{}, Price: {:.3f}",
                contract->ticker, ToString(req.direction), ToString(req.offset), ToString(req.type),
                req.order_id, req.volume, req.price);
  return true;
}

void OrderManagementSystem::CancelOrder(uint64_t order_id) {
  std::unique_lock<SpinLock> lock(spinlock_);
  auto iter = order_map_.find(order_id);
  if (iter == order_map_.end()) {
    spdlog::error("[OMS::CancelOrder] order not found");
    return;
  }
  gateway_->CancelOrder(order_id, iter->second.privdata);
}

void OrderManagementSystem::CancelForTicker(uint32_t ticker_id) {
  std::unique_lock<SpinLock> lock(spinlock_);
  for (const auto& [order_id, order] : order_map_) {
    if (ticker_id == order.req.contract->ticker_id) gateway_->CancelOrder(order_id, order.privdata);
  }
}

void OrderManagementSystem::CancelAll() {
  std::unique_lock<SpinLock> lock(spinlock_);
  for (const auto& [order_id, order] : order_map_) {
    gateway_->CancelOrder(order_id, order.privdata);
  }
}

Gateway* OrderManagementSystem::LoadGateway(const std::string& gtw_lib_file) {
  void* gtw_handle = dlopen(gtw_lib_file.c_str(), RTLD_LAZY);
  if (!gtw_handle) {
    spdlog::error("failed to load gateway from {}", gtw_lib_file);
    return nullptr;
  }
  auto gateway_ctor = reinterpret_cast<GatewayCreateFunc>(dlsym(gtw_handle, "CreateGateway"));
  if (!gateway_ctor) {
    spdlog::error("failed to init gateway: symbol CreateGateway not found，register your gateway");
    return nullptr;
  }
  auto* gateway = gateway_ctor();
  if (!gateway) {
    spdlog::error("failed to create gateway");
    return nullptr;
  }

  return gateway;
}

void OrderManagementSystem::OnAccount(const Account& account) {
  std::unique_lock<SpinLock> lock(spinlock_);
  account_ = account;
  lock.unlock();

  spdlog::info("[OMS::OnAccount] account_id:{} total_asset:{} cash:{} margin:{} frozen:{}",
               account.account_id, account.total_asset, account.cash, account.margin,
               account.frozen);
}

void OrderManagementSystem::OnPositions(std::vector<Position>* positions) {
  for (auto& position : *positions) {
    auto contract = ContractTable::get_by_index(position.ticker_id);
    if (!contract) {
      spdlog::error("contract not found. ticker_id: {}", position.ticker_id);
      exit(-1);
    }

    auto& lp = position.long_pos;
    auto& sp = position.short_pos;
    spdlog::info(
        "[OMS::OnPosition] {}, LongVol:{}, LongYdVol:{}, LongPrice:{:.2f}, LongFrozen:{}, "
        "LongPNL:{}, ShortVol:{}, ShortYdVol:{}, ShortPrice:{:.2f}, ShortFrozen:{}, ShortPNL:{}",
        contract->ticker, lp.holdings, lp.yd_holdings, lp.cost_price, lp.frozen, lp.float_pnl,
        sp.holdings, sp.yd_holdings, sp.cost_price, sp.frozen, sp.float_pnl);

    if (lp.holdings == 0 && lp.frozen == 0 && sp.holdings == 0 && sp.frozen == 0) return;

    pos_calculator_.SetPosition(position);
  }
}

void OrderManagementSystem::OnTick(const TickData& tick) {
  auto contract = ContractTable::get_by_index(tick.ticker_id);
  assert(contract);
  if (!md_pusher_.Publish(contract->ticker, tick)) {
    spdlog::error("failed to publish tick data. ticker: {}", contract->ticker);
  }

  spdlog::trace("[OMS::OnTick] {}  ask:{:.3f}  bid:{:.3f}", contract->ticker, tick.ask[0],
                tick.bid[0]);
}

void OrderManagementSystem::OnTrades(std::vector<Trade>* trades) {}

bool OrderManagementSystem::OnTimer() {
  gateway_->QueryAccount();

  GatewayQueryResult res;
  gateway_->GetQryResultRB()->GetWithBlocking(&res);
  if (res.msg_type == GatewayMsgType::kAccount) {
    OnAccount(std::get<Account>(res.data));
    gateway_->GetQryResultRB()->GetWithBlocking(&res);
    assert(res.msg_type == GatewayMsgType::kAccountEnd);
  } else {
    assert(res.msg_type == GatewayMsgType::kAccountEnd);
  }
  return true;
}

/*
 * 订单被市场接受后通知策略
 * 告知策略order_id，策略可通过此order_id撤单
 */
void OrderManagementSystem::operator()(const OrderAcceptance& rsp) {
  std::unique_lock<SpinLock> lock(spinlock_);
  auto iter = order_map_.find(rsp.order_id);
  if (iter == order_map_.end()) {
    spdlog::warn("[OMS::OnOrderAccepted] order not found. OrderID: {}", rsp.order_id);
    return;
  }

  auto& order = iter->second;
  if (order.accepted) return;

  order.accepted = true;
  rms_->OnOrderAccepted(&order);

  spdlog::info(
      "[OMS::OnOrderAccepted] order accepted. OrderID: {}, {}, {}{}, Volume:{}, Price:{:.2f}, "
      "OrderType:{}",
      rsp.order_id, order.req.contract->ticker, ToString(order.req.direction),
      ToString(order.req.offset), order.req.volume, order.req.price, ToString(order.req.type));
}

void OrderManagementSystem::operator()(const OrderRejection& rsp) {
  std::unique_lock<SpinLock> lock(spinlock_);
  auto iter = order_map_.find(rsp.order_id);
  if (iter == order_map_.end()) {
    spdlog::warn("[OMS::OnOrderRejected] Order not found. OrderID: {}", rsp.order_id);
    return;
  }

  auto& order = iter->second;
  rms_->OnOrderRejected(&order, ERR_REJECTED);

  spdlog::error("[OMS::OnOrderRejected] order rejected. {}. {}, {}{}, Volume:{}, Price:{:.3f}",
                rsp.reason, order.req.contract->ticker, ToString(order.req.direction),
                ToString(order.req.offset), order.req.volume, order.req.price);

  order_map_.erase(iter);
}

void OrderManagementSystem::operator()(const Trade& rsp) {
  if (rsp.trade_type == TradeType::kSecondaryMarket)
    OnSecondaryMarketTraded(rsp);
  else
    OnPrimaryMarketTraded(rsp);
}

void OrderManagementSystem::OnPrimaryMarketTraded(const Trade& rsp) {
  std::unique_lock<SpinLock> lock(spinlock_);
  auto iter = order_map_.find(rsp.order_id);
  if (iter == order_map_.end()) {
    spdlog::warn(
        "[OMS::OnPrimaryMarketTraded] order not found. OrderID:{}, Traded:{}, Price:{:.3f}",
        rsp.order_id, rsp.volume, rsp.price);
    return;
  }

  auto& order = iter->second;
  if (!order.accepted) {
    order.accepted = true;
    rms_->OnOrderAccepted(&order);

    spdlog::info("[OMS::OnOrderAccepted] order accepted. {}, {}, OrderID:{}, Volume:{}",
                 order.req.contract->ticker, ToString(order.req.direction), rsp.order_id,
                 order.req.volume);
  }

  if (rsp.trade_type == TradeType::kAcquireStock) {
    rms_->OnOrderTraded(&order, &rsp);
  } else if (rsp.trade_type == TradeType::kReleaseStock) {
    rms_->OnOrderTraded(&order, &rsp);
  } else if (rsp.trade_type == TradeType::kCashSubstitution) {
    rms_->OnOrderTraded(&order, &rsp);
  } else if (rsp.trade_type == TradeType::kPrimaryMarket) {
    order.traded_volume = rsp.volume;
    rms_->OnOrderTraded(&order, &rsp);
    spdlog::info("[OMS::OnPrimaryMarketTraded] done. {}, {}, Volume:{}", order.req.contract->ticker,
                 ToString(order.req.direction), order.req.volume);
    order_map_.erase(iter);
  }
}

void OrderManagementSystem::OnSecondaryMarketTraded(const Trade& rsp) {
  std::unique_lock<SpinLock> lock(spinlock_);
  auto iter = order_map_.find(rsp.order_id);
  if (iter == order_map_.end()) {
    spdlog::warn(
        "[OMS::OnSecondaryMarketTraded] Order not found. OrderID:{}, Traded:{}, Price:{:.3f}",
        rsp.order_id, rsp.volume, rsp.price);
    return;
  }

  auto& order = iter->second;
  if (!order.accepted) {
    order.accepted = true;
    rms_->OnOrderAccepted(&order);

    spdlog::info(
        "[OMS::OnOrderAccepted] order accepted. OrderID: {}, {}, {}{}, Volume:{}, Price:{:.2f}, "
        "OrderType:{}",
        rsp.order_id, order.req.contract->ticker, ToString(order.req.direction),
        ToString(order.req.offset), order.req.volume, order.req.price, ToString(order.req.type));
  }

  order.traded_volume += rsp.volume;

  spdlog::info(
      "[OMS::OnOrderTraded] order traded. OrderID: {}, {}, {}{}, Traded:{}, Price:{:.3f}, "
      "TotalTraded/Original:{}/{}",
      rsp.order_id, order.req.contract->ticker, ToString(order.req.direction),
      ToString(order.req.offset), rsp.volume, rsp.price, order.traded_volume, order.req.volume);

  rms_->OnOrderTraded(&order, &rsp);

  if (order.traded_volume + order.canceled_volume == order.req.volume) {
    spdlog::info(
        "[OMS::OnOrderTraded] order completed. OrderID:{}, {}, {}{}, Traded/Original: {}/{}",
        rsp.order_id, order.req.contract->ticker, ToString(order.req.direction),
        ToString(order.req.offset), order.traded_volume, order.req.volume);

    // 订单结束，通知风控模块
    rms_->OnOrderCompleted(&order);
    order_map_.erase(iter);
  }
}

void OrderManagementSystem::operator()(const OrderCancellation& rsp) {
  std::unique_lock<SpinLock> lock(spinlock_);
  auto iter = order_map_.find(rsp.order_id);
  if (iter == order_map_.end()) {
    spdlog::warn("[OMS::OnOrderCanceled] Order not found. OrderID:{}", rsp.order_id);
    return;
  }

  auto& order = iter->second;
  order.canceled_volume = rsp.canceled_volume;

  spdlog::info("[OMS::OnOrderCanceled] order canceled. {}, {}{}, OrderID:{}, Canceled:{}",
               order.req.contract->ticker, ToString(order.req.direction),
               ToString(order.req.offset), rsp.order_id, rsp.canceled_volume);

  rms_->OnOrderCanceled(&order, rsp.canceled_volume);

  if (order.traded_volume + order.canceled_volume == order.req.volume) {
    spdlog::info(
        "[OMS::OnOrderCanceled] order completed. {}, {}{}, OrderID:{}, Traded/Original:{}/{}",
        order.req.contract->ticker, ToString(order.req.direction), ToString(order.req.offset),
        rsp.order_id, order.traded_volume, order.req.volume);

    rms_->OnOrderCompleted(&order);
    order_map_.erase(iter);
  }
}

void OrderManagementSystem::operator()(const OrderCancelRejection& rsp) {
  spdlog::warn("[OMS::OnOrderCancelRejected] order cannot be canceled. {}. OrderID: {}", rsp.reason,
               rsp.order_id);
}

}  // namespace ft
