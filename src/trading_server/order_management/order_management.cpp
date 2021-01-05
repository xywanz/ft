// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "trading_server/order_management/order_management.h"

#include <spdlog/spdlog.h>

#include <utility>

#include "ipc/lockfree-queue/queue.h"
#include "ipc/redis_trader_cmd_helper.h"
#include "trading_server/datastruct/contract_table.h"
#include "trading_server/datastruct/error_code.h"
#include "trading_server/datastruct/protocol.h"
#include "trading_server/risk_management/common/fund_manager.h"
#include "trading_server/risk_management/common/no_self_trade.h"
#include "trading_server/risk_management/common/position_manager.h"
#include "trading_server/risk_management/common/strategy_notifier.h"
#include "trading_server/risk_management/common/throttle_rate_limit.h"
#include "trading_server/risk_management/etf/arbitrage_manager.h"
#include "utils/misc.h"

namespace ft {

OMS::OMS() { rms_ = std::make_unique<RiskManagementSystem>(); }

OMS::~OMS() { Close(); }

bool OMS::Login(const Config& config) {
  spdlog::info("***************OMS****************");
  spdlog::info("* version: {}", version());
  spdlog::info("* compiling time: {} {}", __TIME__, __DATE__);
  spdlog::info("********************************************");
  config.Show();

  // 如果有配置contracts file的路径，则尝试从文件加载
  if (!config.contracts_file.empty()) {
    if (!ContractTable::Init(config.contracts_file))
      spdlog::warn("[OMS::OMS] Failed to Init contract table");
  }

  cmd_queue_key_ = config.key_of_cmd_queue;

  gateway_.reset(CreateGateway(config.api));
  if (!gateway_) {
    spdlog::error("[OMS::Login] Failed. Unknown gateway");
    return false;
  }

  if (!gateway_->Login(this, config)) {
    spdlog::error("[OMS::Login] Failed to Login");
    return false;
  }
  spdlog::info("[OMS::Login] Success. Login as {}", config.investor_id);

  if (!ContractTable::inited()) {
    std::vector<Contract> contracts;
    if (!gateway_->QueryContractList(&contracts)) {
      spdlog::error("[OMS::Login] Failed to initialize contract table");
      return false;
    }
    ContractTable::Init(std::move(contracts));
    ContractTable::store("./contracts.csv");
  }

  md_snapshot_.Init();
  if (!gateway_->Subscribe(config.subscription_list)) {
    spdlog::error("[OMS::Login] Failed to Subscribe market data");
    return false;
  }

  Account init_acct{};
  if (!gateway_->QueryAccount(&init_acct)) {
    spdlog::error("[OMS::Login] Failed to query account");
    return false;
  }
  handle_account(&init_acct);

  // query all positions
  std::vector<Position> init_positions;
  portfolio_.Init(ContractTable::size(), true, account_.account_id);
  if (!gateway_->QueryPositionList(&init_positions)) {
    spdlog::error("[OMS::Login] Failed to query positions");
    return false;
  }
  HandlePositions(&init_positions);

  // query trades to update position
  std::vector<Trade> init_trades;
  if (!gateway_->QueryTradeList(&init_trades)) {
    spdlog::error("[OMS::Login] Failed to query trades");
    return false;
  }
  HandleTrades(&init_trades);

  // Init risk manager
  RiskRuleParams risk_params{};
  risk_params.config = &config;
  risk_params.account = &account_;
  risk_params.portfolio = &portfolio_;
  risk_params.order_map = &order_map_;
  risk_params.md_snapshot = &md_snapshot_;
  rms_->AddRule(std::make_shared<FundManager>());
  rms_->AddRule(std::make_shared<PositionManager>());
  rms_->AddRule(std::make_shared<NoSelfTradeRule>());
  rms_->AddRule(std::make_shared<ThrottleRateLimit>());
  if (config.api == "xtp") rms_->AddRule(std::make_shared<ArbitrageManager>());
  if (!config.no_receipt_mode) rms_->AddRule(std::make_shared<StrategyNotifier>());
  if (!rms_->Init(&risk_params)) {
    spdlog::error("[OMS::Login] 风险管理对象初始化失败");
    return false;
  }

  // 启动个线程去定时查询资金账户信息
  timer_thread_ = std::make_unique<std::thread>(std::mem_fn(&OMS::HandleTimer), this);

  spdlog::info("[OMS::Login] Init done");

  is_logon_ = true;
  return true;
}

void OMS::ProcessCmd() {
  if (cmd_queue_key_ > 0)
    ProcessQueueCmd();
  else
    ProcessRedisCmd();
}

void OMS::ProcessRedisCmd() {
  RedisTraderCmdPuller cmd_puller;
  cmd_puller.set_account(account_.account_id);
  spdlog::info("[OMS::run] Start to recv cmd from topic: {}", cmd_puller.get_topic());

  for (;;) {
    auto reply = cmd_puller.Pull();
    if (!reply) continue;

    auto cmd = reinterpret_cast<const TraderCommand*>(reply->element[2]->str);
    ExecuteCmd(*cmd);
  }
}

void OMS::ProcessQueueCmd() {
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
      spdlog::info("OMS::run_with_queue] Failed to create cmd queue");
      abort();
    }

    if ((cmd_queue = LFQueue_open(cmd_queue_key_, te_user_id)) == nullptr) {
      spdlog::info("OMS::run_with_queue] Failed to open cmd queue");
      abort();
    }
  }

  LFQueue_reset(cmd_queue);
  spdlog::info("[OMS::run] Start to recv cmd from queue: {:#x}", cmd_queue_key_);

  TraderCommand cmd{};
  int res;
  for (;;) {
    // 这里没有使用零拷贝的方式，性能影响甚微
    res = LFQueue_pop(cmd_queue, &cmd, nullptr, nullptr);
    if (res != 0) continue;

    ExecuteCmd(cmd);
  }
}

void OMS::Close() {
  if (gateway_) gateway_->Logout();
}

void OMS::ExecuteCmd(const TraderCommand& cmd) {
#ifdef FT_TIME_PERF
  auto start_time = std::chrono::steady_clock::now();
#endif

  if (cmd.magic != TRADER_CMD_MAGIC) {
    spdlog::error("[OMS::run] Recv unknown cmd: error magic num");
    return;
  }

  switch (cmd.type) {
    case CMD_NEW_ORDER: {
      spdlog::debug("new order");
      SendOrder(cmd);
      break;
    }
    case CMD_CANCEL_ORDER: {
      spdlog::debug("cancel order");
      CancelOrder(cmd.cancel_req.order_id);
      break;
    }
    case CMD_CANCEL_TICKER: {
      spdlog::debug("cancel all for ticker");
      CancelForTicker(cmd.cancel_ticker_req.tid);
      break;
    }
    case CMD_CANCEL_ALL: {
      spdlog::debug("cancel all");
      CancelAll();
      break;
    }
    default: {
      spdlog::error("[StrategyEngine::run] Unknown cmd");
      break;
    }
  }

#ifdef FT_TIME_PERF
  auto end_time = std::chrono::steady_clock::now();
  spdlog::info("[OMS::ExecuteCmd] cmd type: {}   time elapsed: {} ns", cmd.type,
               (end_time - start_time).count());
#endif
}

bool OMS::SendOrder(const TraderCommand& cmd) {
  auto contract = ContractTable::get_by_index(cmd.order_req.tid);
  if (!contract) {
    spdlog::error("[OMS::SendOrder] Contract not found");
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
      spdlog::error("[OMS::SendOrder] 风控未通过: {}", ErrorCodeStr(error_code));
      rms_->OnOrderRejected(&order, error_code);
      return false;
    }
  }

  if (!gateway_->SendOrder(req, &order.privdata)) {
    spdlog::error(
        "[OMS::SendOrder] Failed to SendOrder. {}, {}{}, {}, "
        "Volume:{}, Price:{:.3f}",
        contract->ticker, DirectionToStr(req.direction), OffsetToStr(req.offset),
        OrderTypeToStr(req.type), static_cast<int>(req.volume), static_cast<double>(req.price));

    rms_->OnOrderRejected(&order, ERR_SEND_FAILED);
    return false;
  }

  order_map_.emplace(static_cast<uint64_t>(req.order_id), order);
  rms_->OnOrderSent(&order);

  spdlog::debug(
      "[OMS::SendOrder] Success. {}, {}{}, {}, OrderID:{}, "
      "Volume:{}, Price: {:.3f}",
      contract->ticker, DirectionToStr(req.direction), OffsetToStr(req.offset),
      OrderTypeToStr(req.type), static_cast<uint64_t>(req.order_id), static_cast<int>(req.volume),
      static_cast<double>(req.price));
  return true;
}

void OMS::CancelOrder(uint64_t order_id) {
  std::unique_lock<std::mutex> lock(mutex_);
  auto iter = order_map_.find(order_id);
  if (iter == order_map_.end()) {
    spdlog::error("[OMS::CancelOrder] Failed. Order not found");
    return;
  }
  gateway_->CancelOrder(order_id, iter->second.privdata);
}

void OMS::CancelForTicker(uint32_t tid) {
  std::unique_lock<std::mutex> lock(mutex_);
  for (const auto& [order_id, order] : order_map_) {
    if (tid == order.req.contract->tid) gateway_->CancelOrder(order_id, order.privdata);
  }
}

void OMS::CancelAll() {
  std::unique_lock<std::mutex> lock(mutex_);
  for (const auto& [order_id, order] : order_map_) {
    gateway_->CancelOrder(order_id, order.privdata);
  }
}

void OMS::handle_account(Account* account) {
  std::unique_lock<std::mutex> lock(mutex_);
  account_ = *account;
  lock.unlock();

  spdlog::info("[OMS::on_query_account] {}", DumpAccount(*account));
}

void OMS::HandlePositions(std::vector<Position>* positions) {
  for (auto& position : *positions) {
    auto contract = ContractTable::get_by_index(position.tid);
    assert(contract);

    auto& lp = position.long_pos;
    auto& sp = position.short_pos;
    spdlog::info(
        "[OMS::on_query_position] {}, LongVol:{}, LongYdVol:{}, "
        "LongPrice:{:.2f}, LongFrozen:{}, LongPNL:{}, ShortVol:{}, "
        "ShortYdVol:{}, ShortPrice:{:.2f}, ShortFrozen:{}, ShortPNL:{}",
        contract->ticker, lp.holdings, lp.yd_holdings, lp.cost_price, lp.frozen, lp.float_pnl,
        sp.holdings, sp.yd_holdings, sp.cost_price, sp.frozen, sp.float_pnl);

    if (lp.holdings == 0 && lp.frozen == 0 && sp.holdings == 0 && sp.frozen == 0) return;

    portfolio_.set_position(position);
  }
}

void OMS::OnTick(TickData* tick) {
  if (!is_logon_) return;

  auto contract = ContractTable::get_by_index(tick->tid);
  assert(contract);
  md_pusher_.Push(contract->ticker, *tick);

  md_snapshot_.UpdateSnapshot(*tick);
  spdlog::trace("[OMS::process_tick] {}  ask:{:.3f}  bid:{:.3f}", contract->ticker, tick->ask[0],
                tick->bid[0]);
}

void OMS::HandleTrades(std::vector<Trade>* trades) {
  for (auto& trade : *trades) {
    portfolio_.UpdateOnQueryTrade(trade.tid, trade.direction, trade.offset, trade.volume);
  }
}

void OMS::HandleTimer() {
  Account tmp{};
  for (;;) {
    std::this_thread::sleep_for(std::chrono::seconds(15));
    gateway_->QueryAccount(&tmp);
    handle_account(&tmp);
  }
}

/*
 * 订单被市场接受后通知策略
 * 告知策略order_id，策略可通过此order_id撤单
 */
void OMS::OnOrderAccepted(OrderAcceptance* rsp) {
  std::unique_lock<std::mutex> lock(mutex_);
  auto iter = order_map_.find(rsp->order_id);
  if (iter == order_map_.end()) {
    spdlog::warn("[OMS::OnOrderAccepted] Order not found. OrderID: {}", rsp->order_id);
    return;
  }

  auto& order = iter->second;
  if (order.accepted) return;

  order.accepted = true;
  rms_->OnOrderAccepted(&order);

  spdlog::info(
      "[OMS::OnOrderAccepted] 报单委托成功. OrderID: {}, {}, {}{}, "
      "Volume:{}, Price:{:.2f}, OrderType:{}",
      rsp->order_id, order.req.contract->ticker, DirectionToStr(order.req.direction),
      OffsetToStr(order.req.offset), static_cast<int>(order.req.volume),
      static_cast<double>(order.req.price), OrderTypeToStr(order.req.type));
}

void OMS::OnOrderRejected(OrderRejection* rsp) {
  std::unique_lock<std::mutex> lock(mutex_);
  auto iter = order_map_.find(rsp->order_id);
  if (iter == order_map_.end()) {
    spdlog::warn("[OMS::OnOrderRejected] Order not found. OrderID: {}", rsp->order_id);
    return;
  }

  auto& order = iter->second;
  rms_->OnOrderRejected(&order, ERR_REJECTED);

  spdlog::error(
      "[OMS::OnOrderRejected] 报单被拒：{}. {}, {}{}, Volume:{}, "
      "Price:{:.3f}",
      rsp->reason, order.req.contract->ticker, DirectionToStr(order.req.direction),
      OffsetToStr(order.req.offset), static_cast<int>(order.req.volume),
      static_cast<double>(order.req.price));

  order_map_.erase(iter);
}

void OMS::OnOrderTraded(Trade* rsp) {
  if (rsp->trade_type == TradeType::SECONDARY_MARKET)
    OnSecondaryMarketTraded(rsp);
  else
    OnPrimaryMarketTraded(rsp);
}

void OMS::OnPrimaryMarketTraded(Trade* rsp) {
  std::unique_lock<std::mutex> lock(mutex_);
  auto iter = order_map_.find(rsp->order_id);
  if (iter == order_map_.end()) {
    spdlog::warn(
        "[OMS::OnPrimaryMarketTraded] Order not found. "
        "OrderID:{}, Traded:{}, Price:{:.3f}",
        rsp->order_id, rsp->volume, rsp->price);
    return;
  }

  auto& order = iter->second;
  if (!order.accepted) {
    order.accepted = true;
    rms_->OnOrderAccepted(&order);

    spdlog::info(
        "[OMS::OnOrderAccepted] 报单委托成功. {}, {}, "
        "OrderID:{}, Volume:{}",
        order.req.contract->ticker, DirectionToStr(order.req.direction),
        static_cast<uint32_t>(rsp->order_id), static_cast<int>(order.req.volume));
  }

  if (rsp->trade_type == TradeType::ACQUIRED_STOCK) {
    rms_->OnOrderTraded(&order, rsp);
  } else if (rsp->trade_type == TradeType::RELEASED_STOCK) {
    rms_->OnOrderTraded(&order, rsp);
  } else if (rsp->trade_type == TradeType::CASH_SUBSTITUTION) {
    rms_->OnOrderTraded(&order, rsp);
  } else if (rsp->trade_type == TradeType::PRIMARY_MARKET) {
    order.traded_volume = rsp->volume;
    rms_->OnOrderTraded(&order, rsp);
    // risk_mgr_->OnOrderCompleted(&order);
    spdlog::info("[OMS::OnPrimaryMarketTraded] done. {}, {}, Volume:{}", order.req.contract->ticker,
                 DirectionToStr(order.req.direction), static_cast<int>(order.req.volume));
    order_map_.erase(iter);
  }
}

void OMS::OnSecondaryMarketTraded(Trade* rsp) {
  std::unique_lock<std::mutex> lock(mutex_);
  auto iter = order_map_.find(rsp->order_id);
  if (iter == order_map_.end()) {
    spdlog::warn(
        "[OMS::OnSecondaryMarketTraded] Order not found. "
        "OrderID:{}, Traded:{}, Price:{:.3f}",
        rsp->order_id, rsp->volume, rsp->price);
    return;
  }

  auto& order = iter->second;
  if (!order.accepted) {
    order.accepted = true;
    rms_->OnOrderAccepted(&order);

    spdlog::info(
        "[OMS::OnOrderAccepted] 报单委托成功. OrderID: {}, {}, {}{}, "
        "Volume:{}, Price:{:.2f}, OrderType:{}",
        rsp->order_id, order.req.contract->ticker, DirectionToStr(order.req.direction),
        OffsetToStr(order.req.offset), static_cast<int>(order.req.volume),
        static_cast<double>(order.req.price), OrderTypeToStr(order.req.type));
  }

  order.traded_volume += rsp->volume;

  spdlog::info(
      "[OMS::OnOrderTraded] 报单成交. OrderID: {}, {}, {}{}, Traded:{}, "
      "Price:{:.3f}, TotalTraded/Original:{}/{}",
      rsp->order_id, order.req.contract->ticker, DirectionToStr(order.req.direction),
      OffsetToStr(order.req.offset), static_cast<int>(rsp->volume), static_cast<double>(rsp->price),
      static_cast<int>(order.traded_volume), static_cast<int>(order.req.volume));

  rms_->OnOrderTraded(&order, rsp);

  if (order.traded_volume + order.canceled_volume == order.req.volume) {
    spdlog::info(
        "[OMS::OnOrderTraded] 报单完成. OrderID:{}, {}, {}{}, "
        "Traded/Original: {}/{}",
        rsp->order_id, order.req.contract->ticker, DirectionToStr(order.req.direction),
        OffsetToStr(order.req.offset), static_cast<int>(order.traded_volume),
        static_cast<int>(order.req.volume));

    // 订单结束，通知风控模块
    rms_->OnOrderCompleted(&order);
    order_map_.erase(iter);
  }
}

void OMS::OnOrderCanceled(OrderCancellation* rsp) {
  std::unique_lock<std::mutex> lock(mutex_);
  auto iter = order_map_.find(rsp->order_id);
  if (iter == order_map_.end()) {
    spdlog::warn("[OMS::OnOrderCanceled] Order not found. OrderID:{}", rsp->order_id);
    return;
  }

  auto& order = iter->second;
  order.canceled_volume = rsp->canceled_volume;

  spdlog::info(
      "[OMS::OnOrderCanceled] 报单已撤. {}, {}{}, OrderID:{}, "
      "Canceled:{}",
      order.req.contract->ticker, DirectionToStr(order.req.direction),
      OffsetToStr(order.req.offset), rsp->order_id, rsp->canceled_volume);

  rms_->OnOrderCanceled(&order, rsp->canceled_volume);

  if (order.traded_volume + order.canceled_volume == order.req.volume) {
    spdlog::info(
        "[OMS::OnOrderCanceled] 报单完成. {}, {}{}, OrderID:{}, "
        "Traded/Original:{}/{}",
        order.req.contract->ticker, DirectionToStr(order.req.direction),
        OffsetToStr(order.req.offset), static_cast<uint64_t>(rsp->order_id),
        static_cast<int>(order.traded_volume), static_cast<int>(order.req.volume));

    rms_->OnOrderCompleted(&order);
    order_map_.erase(iter);
  }
}

void OMS::OnOrderCancelRejected(OrderCancelRejection* rsp) {
  spdlog::warn("[OMS::OnOrderCancelRejected] 订单不可撤：{}. OrderID: {}", rsp->reason,
               rsp->order_id);
}

}  // namespace ft
