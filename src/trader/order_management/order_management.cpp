// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "trader/order_management/order_management.h"

#include <dlfcn.h>
#include <ft/base/contract_table.h>
#include <ft/component/pubsub/subscriber.h>
#include <ft/utils/config_loader.h>
#include <ft/utils/lockfree-queue/queue.h>
#include <ft/utils/misc.h>
#include <ft/utils/protocol_utils.h>
#include <spdlog/spdlog.h>

#include <utility>

#include "trader/risk_management/common/fund_manager.h"
#include "trader/risk_management/common/no_self_trade.h"
#include "trader/risk_management/common/position_manager.h"
#include "trader/risk_management/common/strategy_notifier.h"
#include "trader/risk_management/common/throttle_rate_limit.h"
#include "trader/risk_management/etf/arbitrage_manager.h"

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
  std::string subs_list = "";
  for (const auto& ticker : cfg.subscription_list) {
    subs_list += ticker + ",";
  }
  spdlog::info("  subscription_list: {}", subs_list);
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

OrderManagementSystem::~OrderManagementSystem() { Close(); }

bool OrderManagementSystem::Login(const Config& config) {
  spdlog::info("***************OrderManagementSystem****************");
  spdlog::info("* version: {}", version());
  spdlog::info("* compiling time: {} {}", __TIME__, __DATE__);
  spdlog::info("****************************************************");
  ShowConfig(config);

  // 如果有配置contracts file的路径，则尝试从文件加载
  if (!config.contracts_file.empty()) {
    if (!ContractTable::Init(config.contracts_file))
      spdlog::warn("从文件初始化ContractTable失败，尝试从Gateway查询。加载路径：{}",
                   config.contracts_file);
  }

  cmd_queue_key_ = config.key_of_cmd_queue;

  void* gateway_dl_handle_ = dlopen(config.api.c_str(), RTLD_LAZY);
  if (!gateway_dl_handle_) {
    spdlog::error("Gateway动态库加载失败。加载路径: {}", config.api);
    return false;
  }
  auto gateway_ctor =
      reinterpret_cast<GatewayCreateFunc>(dlsym(gateway_dl_handle_, "CreateGateway"));
  if (!gateway_ctor) {
    spdlog::error(
        "CreateGateway函数未找到，Gateway初始化失败，请检查在Gateway实现中是否调用了REGISTER_"
        "GATEWAY进行注册。加载路径：{}",
        config.api);
    return false;
  }
  auto gateway_dtor_ =
      reinterpret_cast<GatewayDestroyFunc>(dlsym(gateway_dl_handle_, "DestroyGateway"));
  if (!gateway_dtor_) {
    spdlog::error(
        "DestroyGateway函数未找到，Gateway初始化失败，请检查在Gateway实现中是否调用了REGISTER_"
        "GATEWAY进行注册。加载路径：{}",
        config.api);
    return false;
  }
  gateway_ = gateway_ctor();
  if (!gateway_) {
    spdlog::error("Gateway创建失败");
    return false;
  }

  if (!gateway_->Login(this, config)) {
    spdlog::error("Gateway登录失败");
    return false;
  }
  spdlog::info("Gateway登录成功，当前账户为：{}", config.investor_id);

  if (!ContractTable::is_inited()) {
    std::vector<Contract> contracts;
    if (!gateway_->QueryContractList(&contracts)) {
      spdlog::error("查询合约失败，初始化ContractTable失败");
      return false;
    }
    ContractTable::Init(std::move(contracts));
    ContractTable::Store("./contracts.csv");
  }

  if (!gateway_->Subscribe(config.subscription_list)) {
    spdlog::error("行情订阅失败，请检查订阅列表是否合法，以及订阅列表在ContractTable中");
    return false;
  }

  Account init_acct{};
  if (!gateway_->QueryAccount(&init_acct)) {
    spdlog::error("账户查询失败");
    return false;
  }
  HandleAccount(&init_acct);

  // query all positions
  redis_pos_updater_.SetAccount(account_.account_id);
  redis_pos_updater_.Clear();
  pos_calculator_.SetCallback([this](const Position& new_pos) {
    auto* contract = ContractTable::get_by_index(new_pos.ticker_id);
    if (!contract) {
      spdlog::error("更新Redis仓位失败，合约未找到。ticker_id:{}", new_pos.ticker_id);
      return;
    }
    redis_pos_updater_.set(contract->ticker, new_pos);
  });

  std::vector<Position> init_positions;
  if (!gateway_->QueryPositionList(&init_positions)) {
    spdlog::error("仓位查询失败");
    return false;
  }
  HandlePositions(&init_positions);

  // query trades to update position
  std::vector<Trade> init_trades;
  if (!gateway_->QueryTradeList(&init_trades)) {
    spdlog::error("历史成交查询失败");
    return false;
  }
  HandleTrades(&init_trades);

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
  if (config.api == "xtp") rms_->AddRule(std::make_shared<ArbitrageManager>());
  if (!config.no_receipt_mode) rms_->AddRule(std::make_shared<StrategyNotifier>());
  if (!rms_->Init(&risk_params)) {
    spdlog::error("风控对象初始化失败");
    return false;
  }

  // 启动个线程去定时查询资金账户信息
  timer_thread_.AddTask(15 * 1000, std::mem_fn(&OrderManagementSystem::HandleTimer), this);
  timer_thread_.Start();

  spdlog::info("trading_server初始化成功");

  is_logon_ = true;
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
    spdlog::error("无法订阅订单请求");
    exit(EXIT_FAILURE);
  }

  spdlog::info("trading_server开始从ipc://trade.ft_trader.ipc topic={}接收请求", ft_cmd_topic);
  cmd_sub.Start();
}

void OrderManagementSystem::ProcessQueueCmd() {
  // 创建Queue的时候存在隐患，如果该key之前被其他应用创建且没有释放，
  // OMS还是会依附在这个key的共享内存上，大概率导致内存访问越界。
  // 如果都是同版本的OMS创建的则没有问题，可以重复使用。
  // 释放队列暂时需要手动释放，请使用ipcrm。
  // TODO(kevin): 在LFQueue_create和LFQueue_open的时候加上检验信息。 Done!

  // user_id用于验证是否是trading engine创建的queue
  uint32_t te_user_id = static_cast<uint32_t>(version());
  LFQueue* cmd_queue;
  if ((cmd_queue = LFQueue_open(cmd_queue_key_, te_user_id)) == nullptr) {
    int res = LFQueue_create(cmd_queue_key_, te_user_id, sizeof(TraderCommand), 4096 * 4, false);
    if (res != 0) {
      spdlog::info("订单请求队列创建失败，请检查共享内存key是否被其他程序占用：{:#x}",
                   cmd_queue_key_);
      abort();
    }

    if ((cmd_queue = LFQueue_open(cmd_queue_key_, te_user_id)) == nullptr) {
      spdlog::info("订单请求队列无法打开，请检查共享内存key是否被其他程序占用：{:#x}",
                   cmd_queue_key_);
      abort();
    }
  }

  LFQueue_reset(cmd_queue);
  spdlog::info("开始从共享内存队列接收请求，队列的key为：{:#x}", cmd_queue_key_);

  TraderCommand cmd{};
  int res;
  for (;;) {
    // 这里没有使用零拷贝的方式，性能影响甚微
    res = LFQueue_pop(cmd_queue, &cmd, nullptr, nullptr);
    if (res != 0) continue;

    ExecuteCmd(cmd);
  }
}

void OrderManagementSystem::Close() {
  if (gateway_) {
    gateway_->Logout();
    gateway_ = nullptr;

    gateway_dtor_(gateway_);
    gateway_dtor_ = nullptr;

    dlclose(gateway_dl_handle_);
    gateway_dl_handle_ = nullptr;
  }
}

void OrderManagementSystem::ExecuteCmd(const TraderCommand& cmd) {
#ifdef FT_TIME_PERF
  auto start_time = std::chrono::steady_clock::now();
#endif

  if (cmd.magic != kTradingCmdMagic) {
    spdlog::error("收到未知的指令：非法的magic");
    return;
  }

  switch (cmd.type) {
    case CMD_NEW_ORDER: {
      spdlog::debug("收到新的订单请求");
      SendOrder(cmd);
      break;
    }
    case CMD_CANCEL_ORDER: {
      spdlog::debug("收到撤单请求");
      CancelOrder(cmd.cancel_req.order_id);
      break;
    }
    case CMD_CANCEL_TICKER: {
      spdlog::debug("收到ticker撤单请求");
      CancelForTicker(cmd.cancel_ticker_req.ticker_id);
      break;
    }
    case CMD_CANCEL_ALL: {
      spdlog::debug("收到全撤请求");
      CancelAll();
      break;
    }
    case CMD_NOTIFY: {
      spdlog::debug("收到notify消息。signal={}", cmd.notification.signal);
      gateway_->OnNotify(cmd.notification.signal);
      break;
    }
    default: {
      spdlog::error("未知的请求类型");
      break;
    }
  }

#ifdef FT_TIME_PERF
  auto end_time = std::chrono::steady_clock::now();
  spdlog::info("[OrderManagementSystem::ExecuteCmd] cmd type: {}   time elapsed: {} ns", cmd.type,
               (end_time - start_time).count());
#endif
}

bool OrderManagementSystem::SendOrder(const TraderCommand& cmd) {
  auto contract = ContractTable::get_by_index(cmd.order_req.ticker_id);
  if (!contract) {
    spdlog::error("[OrderManagementSystem::SendOrder] Contract not found");
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

  std::unique_lock<std::mutex> lock(mutex_);
  // 增加是否经过风控检查字段，在紧急情况下可以设置该字段绕过风控下单
  if (!cmd.order_req.without_check) {
    int error_code = rms_->CheckOrderRequest(&order);
    if (error_code != NO_ERROR) {
      spdlog::error("[OrderManagementSystem::SendOrder] 风控未通过: {}", ErrorCodeStr(error_code));
      rms_->OnOrderRejected(&order, error_code);
      return false;
    }
  }

  if (!gateway_->SendOrder(req, &order.privdata)) {
    spdlog::error(
        "[OrderManagementSystem::SendOrder] Failed to SendOrder. {}, {}{}, {}, "
        "Volume:{}, Price:{:.3f}",
        contract->ticker, ToString(req.direction), ToString(req.offset), ToString(req.type),
        static_cast<int>(req.volume), static_cast<double>(req.price));

    rms_->OnOrderRejected(&order, ERR_SEND_FAILED);
    return false;
  }

  order_map_.emplace(static_cast<uint64_t>(req.order_id), order);
  rms_->OnOrderSent(&order);

  spdlog::debug(
      "[OrderManagementSystem::SendOrder] Success. {}, {}{}, {}, OrderID:{}, "
      "Volume:{}, Price: {:.3f}",
      contract->ticker, ToString(req.direction), ToString(req.offset), ToString(req.type),
      static_cast<uint64_t>(req.order_id), static_cast<int>(req.volume),
      static_cast<double>(req.price));
  return true;
}

void OrderManagementSystem::CancelOrder(uint64_t order_id) {
  std::unique_lock<std::mutex> lock(mutex_);
  auto iter = order_map_.find(order_id);
  if (iter == order_map_.end()) {
    spdlog::error("[OrderManagementSystem::CancelOrder] Failed. Order not found");
    return;
  }
  gateway_->CancelOrder(order_id, iter->second.privdata);
}

void OrderManagementSystem::CancelForTicker(uint32_t ticker_id) {
  std::unique_lock<std::mutex> lock(mutex_);
  for (const auto& [order_id, order] : order_map_) {
    if (ticker_id == order.req.contract->ticker_id) gateway_->CancelOrder(order_id, order.privdata);
  }
}

void OrderManagementSystem::CancelAll() {
  std::unique_lock<std::mutex> lock(mutex_);
  for (const auto& [order_id, order] : order_map_) {
    gateway_->CancelOrder(order_id, order.privdata);
  }
}

void OrderManagementSystem::HandleAccount(Account* account) {
  std::unique_lock<std::mutex> lock(mutex_);
  account_ = *account;
  lock.unlock();

  spdlog::info(
      "[OrderManagementSystem::on_query_account] account_id:{} total_asset:{} cash:{} margin:{} "
      "frozen:{}",
      static_cast<uint64_t>(account->account_id), static_cast<double>(account->total_asset),
      static_cast<double>(account->cash), static_cast<double>(account->margin),
      static_cast<double>(account->frozen));
}

void OrderManagementSystem::HandlePositions(std::vector<Position>* positions) {
  for (auto& position : *positions) {
    auto contract = ContractTable::get_by_index(position.ticker_id);
    if (!contract) {
      spdlog::error("Cannot find contract with ticker_id={}",
                    static_cast<uint32_t>(position.ticker_id));
      exit(-1);
    }

    auto& lp = position.long_pos;
    auto& sp = position.short_pos;
    spdlog::info(
        "[OrderManagementSystem::on_query_position] {}, LongVol:{}, LongYdVol:{}, "
        "LongPrice:{:.2f}, LongFrozen:{}, LongPNL:{}, ShortVol:{}, "
        "ShortYdVol:{}, ShortPrice:{:.2f}, ShortFrozen:{}, ShortPNL:{}",
        contract->ticker, static_cast<int>(lp.holdings), static_cast<int>(lp.yd_holdings),
        static_cast<double>(lp.cost_price), static_cast<double>(lp.frozen),
        static_cast<double>(lp.float_pnl), static_cast<int>(sp.holdings),
        static_cast<int>(sp.yd_holdings), static_cast<double>(sp.cost_price),
        static_cast<double>(sp.frozen), static_cast<double>(sp.float_pnl));

    if (lp.holdings == 0 && lp.frozen == 0 && sp.holdings == 0 && sp.frozen == 0) return;

    pos_calculator_.SetPosition(position);
  }
}

void OrderManagementSystem::OnTick(TickData* tick) {
  if (!is_logon_) return;

  auto contract = ContractTable::get_by_index(tick->ticker_id);
  assert(contract);
  if (!md_pusher_.Publish(contract->ticker, *tick)) {
    spdlog::error("failed to publish tick data. ticker:{}", contract->ticker);
  }

  spdlog::trace("[OrderManagementSystem::process_tick] {}  ask:{:.3f}  bid:{:.3f}",
                contract->ticker, tick->ask[0], tick->bid[0]);
}

void OrderManagementSystem::HandleTrades(std::vector<Trade>* trades) {}

bool OrderManagementSystem::HandleTimer() {
  Account tmp{};
  gateway_->QueryAccount(&tmp);
  HandleAccount(&tmp);
  return true;
}

/*
 * 订单被市场接受后通知策略
 * 告知策略order_id，策略可通过此order_id撤单
 */
void OrderManagementSystem::OnOrderAccepted(OrderAcceptance* rsp) {
  std::unique_lock<std::mutex> lock(mutex_);
  auto iter = order_map_.find(rsp->order_id);
  if (iter == order_map_.end()) {
    spdlog::warn("[OrderManagementSystem::OnOrderAccepted] Order not found. OrderID: {}",
                 rsp->order_id);
    return;
  }

  auto& order = iter->second;
  if (order.accepted) return;

  order.accepted = true;
  rms_->OnOrderAccepted(&order);

  spdlog::info(
      "[OrderManagementSystem::OnOrderAccepted] 报单委托成功. OrderID: {}, {}, {}{}, "
      "Volume:{}, Price:{:.2f}, OrderType:{}",
      rsp->order_id, order.req.contract->ticker, ToString(order.req.direction),
      ToString(order.req.offset), static_cast<int>(order.req.volume),
      static_cast<double>(order.req.price), ToString(order.req.type));
}

void OrderManagementSystem::OnOrderRejected(OrderRejection* rsp) {
  std::unique_lock<std::mutex> lock(mutex_);
  auto iter = order_map_.find(rsp->order_id);
  if (iter == order_map_.end()) {
    spdlog::warn("[OrderManagementSystem::OnOrderRejected] Order not found. OrderID: {}",
                 rsp->order_id);
    return;
  }

  auto& order = iter->second;
  rms_->OnOrderRejected(&order, ERR_REJECTED);

  spdlog::error(
      "[OrderManagementSystem::OnOrderRejected] 报单被拒：{}. {}, {}{}, Volume:{}, "
      "Price:{:.3f}",
      rsp->reason, order.req.contract->ticker, ToString(order.req.direction),
      ToString(order.req.offset), static_cast<int>(order.req.volume),
      static_cast<double>(order.req.price));

  order_map_.erase(iter);
}

void OrderManagementSystem::OnOrderTraded(Trade* rsp) {
  if (rsp->trade_type == TradeType::kSecondaryMarket)
    OnSecondaryMarketTraded(rsp);
  else
    OnPrimaryMarketTraded(rsp);
}

void OrderManagementSystem::OnPrimaryMarketTraded(Trade* rsp) {
  std::unique_lock<std::mutex> lock(mutex_);
  auto iter = order_map_.find(rsp->order_id);
  if (iter == order_map_.end()) {
    spdlog::warn(
        "[OrderManagementSystem::OnPrimaryMarketTraded] Order not found. "
        "OrderID:{}, Traded:{}, Price:{:.3f}",
        rsp->order_id, rsp->volume, rsp->price);
    return;
  }

  auto& order = iter->second;
  if (!order.accepted) {
    order.accepted = true;
    rms_->OnOrderAccepted(&order);

    spdlog::info(
        "[OrderManagementSystem::OnOrderAccepted] 报单委托成功. {}, {}, "
        "OrderID:{}, Volume:{}",
        order.req.contract->ticker, ToString(order.req.direction),
        static_cast<uint32_t>(rsp->order_id), static_cast<int>(order.req.volume));
  }

  if (rsp->trade_type == TradeType::kAcquireStock) {
    rms_->OnOrderTraded(&order, rsp);
  } else if (rsp->trade_type == TradeType::kReleaseStock) {
    rms_->OnOrderTraded(&order, rsp);
  } else if (rsp->trade_type == TradeType::kCashSubstitution) {
    rms_->OnOrderTraded(&order, rsp);
  } else if (rsp->trade_type == TradeType::kPrimaryMarket) {
    order.traded_volume = rsp->volume;
    rms_->OnOrderTraded(&order, rsp);
    // risk_mgr_->OnOrderCompleted(&order);
    spdlog::info("[OrderManagementSystem::OnPrimaryMarketTraded] done. {}, {}, Volume:{}",
                 order.req.contract->ticker, ToString(order.req.direction),
                 static_cast<int>(order.req.volume));
    order_map_.erase(iter);
  }
}

void OrderManagementSystem::OnSecondaryMarketTraded(Trade* rsp) {
  std::unique_lock<std::mutex> lock(mutex_);
  auto iter = order_map_.find(rsp->order_id);
  if (iter == order_map_.end()) {
    spdlog::warn(
        "[OrderManagementSystem::OnSecondaryMarketTraded] Order not found. "
        "OrderID:{}, Traded:{}, Price:{:.3f}",
        rsp->order_id, rsp->volume, rsp->price);
    return;
  }

  auto& order = iter->second;
  if (!order.accepted) {
    order.accepted = true;
    rms_->OnOrderAccepted(&order);

    spdlog::info(
        "[OrderManagementSystem::OnOrderAccepted] 报单委托成功. OrderID: {}, {}, {}{}, "
        "Volume:{}, Price:{:.2f}, OrderType:{}",
        rsp->order_id, order.req.contract->ticker, ToString(order.req.direction),
        ToString(order.req.offset), static_cast<int>(order.req.volume),
        static_cast<double>(order.req.price), ToString(order.req.type));
  }

  order.traded_volume += rsp->volume;

  spdlog::info(
      "[OrderManagementSystem::OnOrderTraded] 报单成交. OrderID: {}, {}, {}{}, Traded:{}, "
      "Price:{:.3f}, TotalTraded/Original:{}/{}",
      rsp->order_id, order.req.contract->ticker, ToString(order.req.direction),
      ToString(order.req.offset), static_cast<int>(rsp->volume), static_cast<double>(rsp->price),
      static_cast<int>(order.traded_volume), static_cast<int>(order.req.volume));

  rms_->OnOrderTraded(&order, rsp);

  if (order.traded_volume + order.canceled_volume == order.req.volume) {
    spdlog::info(
        "[OrderManagementSystem::OnOrderTraded] 报单完成. OrderID:{}, {}, {}{}, "
        "Traded/Original: {}/{}",
        rsp->order_id, order.req.contract->ticker, ToString(order.req.direction),
        ToString(order.req.offset), static_cast<int>(order.traded_volume),
        static_cast<int>(order.req.volume));

    // 订单结束，通知风控模块
    rms_->OnOrderCompleted(&order);
    order_map_.erase(iter);
  }
}

void OrderManagementSystem::OnOrderCanceled(OrderCancellation* rsp) {
  std::unique_lock<std::mutex> lock(mutex_);
  auto iter = order_map_.find(rsp->order_id);
  if (iter == order_map_.end()) {
    spdlog::warn("[OrderManagementSystem::OnOrderCanceled] Order not found. OrderID:{}",
                 rsp->order_id);
    return;
  }

  auto& order = iter->second;
  order.canceled_volume = rsp->canceled_volume;

  spdlog::info(
      "[OrderManagementSystem::OnOrderCanceled] 报单已撤. {}, {}{}, OrderID:{}, "
      "Canceled:{}",
      order.req.contract->ticker, ToString(order.req.direction), ToString(order.req.offset),
      rsp->order_id, rsp->canceled_volume);

  rms_->OnOrderCanceled(&order, rsp->canceled_volume);

  if (order.traded_volume + order.canceled_volume == order.req.volume) {
    spdlog::info(
        "[OrderManagementSystem::OnOrderCanceled] 报单完成. {}, {}{}, OrderID:{}, "
        "Traded/Original:{}/{}",
        order.req.contract->ticker, ToString(order.req.direction), ToString(order.req.offset),
        static_cast<uint64_t>(rsp->order_id), static_cast<int>(order.traded_volume),
        static_cast<int>(order.req.volume));

    rms_->OnOrderCompleted(&order);
    order_map_.erase(iter);
  }
}

void OrderManagementSystem::OnOrderCancelRejected(OrderCancelRejection* rsp) {
  spdlog::warn("[OrderManagementSystem::OnOrderCancelRejected] 订单不可撤：{}. OrderID: {}",
               rsp->reason, rsp->order_id);
}

}  // namespace ft
