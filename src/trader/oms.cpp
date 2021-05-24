// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "trader/oms.h"

#include <dlfcn.h>

#include <utility>

#include "ft/base/contract_table.h"
#include "ft/base/log.h"
#include "ft/component/yijinjing/journal/Timer.h"
#include "ft/utils/misc.h"
#include "ft/utils/protocol_utils.h"
#include "trader/risk/common/fund_manager.h"
#include "trader/risk/common/no_self_trade.h"
#include "trader/risk/common/position_manager.h"
#include "trader/risk/common/throttle_rate_limit.h"

namespace ft {

OrderManagementSystem::OrderManagementSystem() { rms_ = std::make_unique<RiskManagementSystem>(); }

bool OrderManagementSystem::Init(const FlareTraderConfig& config) {
  LOG_INFO("OMS compiling time: {} {}", __TIME__, __DATE__);

  config_ = &config;

  if (!InitContractTable()) {
    return false;
  }

  if (!InitMQ()) {
    return false;
  }

  if (!InitGateway()) {
    return false;
  }
  std::this_thread::sleep_for(std::chrono::seconds(1));

  if (!InitAccount()) {
    return false;
  }
  std::this_thread::sleep_for(std::chrono::seconds(1));

  if (!InitPositions()) {
    return false;
  }
  std::this_thread::sleep_for(std::chrono::seconds(1));

  if (!InitTradeInfo()) {
    return false;
  }
  std::this_thread::sleep_for(std::chrono::seconds(1));

  if (!InitRMS()) {
    return false;
  }

  if (!SubscribeMarketData()) {
    return false;
  }

  // 启动个线程去定时查询资金账户信息
  timer_thread_.AddTask(15 * 1000, std::mem_fn(&OrderManagementSystem::OnTimer), this);
  timer_thread_.Start();

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

  LOG_INFO("ft_trader inited");
  is_logon_ = true;

  return true;
}

void OrderManagementSystem::ProcessCmd() {
  for (;;) {
    for (auto& reader : trade_msg_readers_) {
      auto frame = reader->getNextFrame();
      if (frame) {
        auto* data = reinterpret_cast<char*>(frame->getData());
        TraderCommand cmd;
        cmd.ParseFromString(data, frame->getDataLength());
        ExecuteCmd(cmd);
      }
    }
  }
}

void OrderManagementSystem::ExecuteCmd(const TraderCommand& cmd) {
  if (cmd.magic != kTradingCmdMagic) {
    LOG_ERROR("[OMS::ExecuteCmd] invalid magic number of cmd");
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
      LOG_ERROR("[OMS::ExecuteCmd] unknown cmd");
      break;
    }
  }
}

bool OrderManagementSystem::SendOrder(const TraderCommand& cmd) {
  auto contract = ContractTable::get_by_index(cmd.order_req.ticker_id);
  if (!contract) {
    LOG_ERROR("[OMS::SendOrder] contract not found. ticker_id:{}", cmd.order_req.ticker_id);
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
      LOG_ERROR("[OMS::SendOrder] risk: {}", ErrorCodeStr(error_code));
      rms_->OnOrderRejected(&order, error_code);
      SendRspToStrategy(order, 0, 0.0, error_code);
      return false;
    }
  }

  if (!gateway_->SendOrder(req, &order.privdata)) {
    LOG_ERROR("[OMS::SendOrder] failed to send order. {}, {}{}, {}, Volume:{}, Price:{:.3f}",
              contract->ticker, ToString(req.direction), ToString(req.offset), ToString(req.type),
              req.volume, req.price);

    rms_->OnOrderRejected(&order, ERR_SEND_FAILED);
    SendRspToStrategy(order, 0, 0.0, ERR_SEND_FAILED);
    return false;
  }

  order_map_.emplace(req.order_id, order);
  rms_->OnOrderSent(&order);

  LOG_DEBUG("[OMS::SendOrder] success. OrderID:{}, {}, {}{}, {}, Volume:{}, Price:{:.3f}",
            req.order_id, contract->ticker, ToString(req.direction), ToString(req.offset),
            ToString(req.type), , req.volume, req.price);
  return true;
}

void OrderManagementSystem::CancelOrder(uint64_t order_id) {
  std::unique_lock<SpinLock> lock(spinlock_);
  auto iter = order_map_.find(order_id);
  if (iter == order_map_.end()) {
    LOG_ERROR("[OMS::CancelOrder] order not found. order_id:{}", order_id);
    return;
  }
  gateway_->CancelOrder(order_id, iter->second.privdata);
}

void OrderManagementSystem::CancelForTicker(uint32_t ticker_id) {
  std::unique_lock<SpinLock> lock(spinlock_);
  for (const auto& [order_id, order] : order_map_) {
    if (ticker_id == order.req.contract->ticker_id) {
      gateway_->CancelOrder(order_id, order.privdata);
    }
  }
}

void OrderManagementSystem::CancelAll() {
  std::unique_lock<SpinLock> lock(spinlock_);
  for (const auto& [order_id, order] : order_map_) {
    gateway_->CancelOrder(order_id, order.privdata);
  }
}

bool OrderManagementSystem::InitGateway() {
  gateway_ = CreateGateway(config_->gateway_config.api);
  if (!gateway_) {
    LOG_ERROR("[OMS::InitGateway] failed to create gateway");
    return false;
  }

  if (!gateway_->Init(config_->gateway_config)) {
    LOG_ERROR("[OMS::InitGateway] failed to init gateway");
    return false;
  }
  LOG_INFO("[OMS::InitGateway] gateway inited");
  return true;
}

bool OrderManagementSystem::InitContractTable() {
  if (!ContractTable::Init(config_->global_config.contract_file)) {
    LOG_ERROR("[OMS::InitContractTable] failed to init contract table");
    return false;
  }
  return true;
}

bool OrderManagementSystem::InitAccount() {
  auto qry_res_rb = gateway_->GetQryResultRB();
  GatewayQueryResult qry_res;

  if (!gateway_->QueryAccount()) {
    LOG_ERROR("[OMS::InitAccount] failed to query account");
    return false;
  }
  qry_res_rb->GetWithBlocking(&qry_res);
  if (qry_res.msg_type != GatewayMsgType::kAccount) {
    LOG_ERROR("[OMS::InitAccount] error occurred when querying account");
    return false;
  }
  OnAccount(std::get<Account>(qry_res.data));
  qry_res_rb->GetWithBlocking(&qry_res);
  assert(qry_res.msg_type == GatewayMsgType::kAccountEnd);
  return true;
}

bool OrderManagementSystem::InitPositions() {
  auto qry_res_rb = gateway_->GetQryResultRB();
  GatewayQueryResult qry_res;

  // query all positions
  redis_pos_updater_.SetAccount(account_.account_id);
  redis_pos_updater_.Clear();
  pos_calculator_.SetCallback([this](const Position& new_pos) {
    auto* contract = ContractTable::get_by_index(new_pos.ticker_id);
    if (!contract) {
      LOG_ERROR(
          "[OMS::::InitPositions] contract not found. failed to update positions in redis. "
          "ticker_id:{}",
          new_pos.ticker_id);
      return;
    }
    redis_pos_updater_.set(contract->ticker, new_pos);
  });

  std::vector<Position> init_positions;
  if (!gateway_->QueryPositions()) {
    LOG_ERROR("[OMS::InitPositions] failed to query positions");
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
  return true;
}

bool OrderManagementSystem::InitTradeInfo() {
  auto qry_res_rb = gateway_->GetQryResultRB();
  GatewayQueryResult qry_res;

  // query trades to update position
  std::vector<Trade> init_trades;
  if (!gateway_->QueryTrades()) {
    LOG_ERROR("[OMS::InitTradeInfo] failed to query trades");
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
  return true;
}

bool OrderManagementSystem::InitRMS() {
  RiskRuleParams risk_params{};
  risk_params.config = &config_->rms_config;
  risk_params.account = &account_;
  risk_params.pos_calculator = &pos_calculator_;
  risk_params.order_map = &order_map_;
  rms_->AddRule(std::make_shared<FundManager>());
  rms_->AddRule(std::make_shared<PositionManager>());
  rms_->AddRule(std::make_shared<NoSelfTradeRule>());
  rms_->AddRule(std::make_shared<ThrottleRateLimit>());
  if (!rms_->Init(&risk_params)) {
    LOG_ERROR("[OMS::InitRMS] failed to init rms");
    return false;
  }
  return true;
}

bool OrderManagementSystem::InitMQ() {
  for (auto& strategy_conf : config_->strategy_config_list) {
    if (strategy_conf.strategy_name.size() >= sizeof(StrategyIdType)) {
      LOG_ERROR("[OMS::InitMQ] max len of stratey name is {}", sizeof(StrategyIdType) - 1);
      return false;
    }
    auto rsp_writer =
        yijinjing::JournalWriter::create(".", strategy_conf.rsp_mq_name, "rsp_writer");
    rsp_writers_.emplace_back(rsp_writer);
    strategy_name_to_index_.emplace(strategy_conf.strategy_name, rsp_writers_.size() - 1);

    auto trade_msg_reader = yijinjing::JournalReader::create(
        ".", strategy_conf.trade_mq_name, yijinjing::getNanoTime(), "trade_msg_reader");
    trade_msg_readers_.emplace_back(trade_msg_reader);

    std::set<std::string> sub_set(strategy_conf.subscription_list.begin(),
                                  strategy_conf.subscription_list.end());
    if (!sub_set.empty()) {
      auto md_writer =
          yijinjing::JournalWriter::create(".", strategy_conf.md_mq_name, "oms_md_writer");
      for (auto& ticker : sub_set) {
        auto* contract = ContractTable::get_by_ticker(ticker);
        if (!contract) {
          LOG_ERROR("[OMS::InitMQ] failed to subscribe market data {}. contract not found.",
                    ticker);
          return false;
        }
        md_dispatch_map_[contract->ticker_id].emplace_back(md_writer);
      }
      subscription_set_.merge(sub_set);
    }
  }

  return true;
}

bool OrderManagementSystem::SubscribeMarketData() {
  std::vector<std::string> sub_list;
  for (auto& ticker : subscription_set_) {
    sub_list.emplace_back(ticker);
  }
  if (!gateway_->Subscribe(sub_list)) {
    LOG_ERROR("[OMS::SubscribeMarketData] failed to subscribe market data");
    return false;
  }
  return true;
}

void OrderManagementSystem::SendRspToStrategy(const Order& order, int this_traded, double price,
                                              int error_code) {
  auto it = strategy_name_to_index_.find(order.strategy_id);
  if (it == strategy_name_to_index_.end()) {
    LOG_WARN("[OMS::SendRspToStrategy] failed to send rsp: unknown strategy name");
    return;
  }
  auto index = it->second;

  OrderResponse rsp{};
  rsp.client_order_id = order.client_order_id;
  rsp.order_id = order.req.order_id;
  rsp.ticker_id = order.req.contract->ticker_id;
  rsp.direction = order.req.direction;
  rsp.offset = order.req.offset;
  rsp.original_volume = order.req.volume;
  rsp.traded_volume = order.traded_volume;
  rsp.price = order.req.price;
  rsp.this_traded = this_traded;
  rsp.this_traded_price = price;
  rsp.completed = order.canceled_volume + order.traded_volume == order.req.volume;
  rsp.error_code = error_code;

  std::string buf;
  rsp.SerializeToString(&buf);
  rsp_writers_[index]->write_str(buf, 0);
}

void OrderManagementSystem::OnAccount(const Account& account) {
  std::unique_lock<SpinLock> lock(spinlock_);
  account_ = account;
  lock.unlock();

  LOG_DEBUG("[OMS::OnAccount] account_id:{} total_asset:{} cash:{} margin:{} frozen:{}",
            account.account_id, account.total_asset, account.cash, account.margin, account.frozen);
}

void OrderManagementSystem::OnPositions(std::vector<Position>* positions) {
  for (auto& position : *positions) {
    auto contract = ContractTable::get_by_index(position.ticker_id);
    if (!contract) {
      LOG_ERROR("contract not found. ticker_id: {}", position.ticker_id);
      exit(-1);
    }

    auto& lp = position.long_pos;
    auto& sp = position.short_pos;
    LOG_INFO(
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

  auto& writers = md_dispatch_map_[contract->ticker_id];
  std::string buf;
  tick.SerializeToString(&buf);
  for (auto& writer : writers) {
    writer->write_str(buf, 0);
  }

  LOG_TRACE("[OMS::OnTick] {}  ask:{:.3f}  bid:{:.3f}", contract->ticker, tick.ask[0], tick.bid[0]);
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
    LOG_WARN("[OMS::OnOrderAccepted] order not found. OrderID: {}", rsp.order_id);
    return;
  }

  auto& order = iter->second;
  if (order.accepted) return;

  order.accepted = true;
  rms_->OnOrderAccepted(&order);
  SendRspToStrategy(order, 0, 0.0, NO_ERROR);

  LOG_INFO(
      "[OMS::OnOrderAccepted] order accepted. OrderID:{}, {}, {}{}, {}, Volume:{}, Price:{:.2f}",
      rsp.order_id, order.req.contract->ticker, ToString(order.req.direction),
      ToString(order.req.offset), ToString(order.req.type), order.req.volume, order.req.price);
}

void OrderManagementSystem::operator()(const OrderRejection& rsp) {
  std::unique_lock<SpinLock> lock(spinlock_);
  auto iter = order_map_.find(rsp.order_id);
  if (iter == order_map_.end()) {
    LOG_WARN("[OMS::OnOrderRejected] order not found. order_id:{}", rsp.order_id);
    return;
  }

  auto& order = iter->second;
  rms_->OnOrderRejected(&order, ERR_REJECTED);
  SendRspToStrategy(order, 0, 0.0, ERR_REJECTED);

  LOG_ERROR("[OMS::OnOrderRejected] order rejected. {}. {}, {}{}, {}, Volume:{}, Price:{:.3f}",
            rsp.reason, order.req.contract->ticker, ToString(order.req.direction),
            ToString(order.req.offset), ToString(order.req.type), order.req.volume,
            order.req.price);

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
    LOG_WARN("[OMS::OnPrimaryMarketTraded] order not found. OrderID:{}, Traded:{}, Price:{:.3f}",
             rsp.order_id, rsp.volume, rsp.price);
    return;
  }

  auto& order = iter->second;
  if (!order.accepted) {
    order.accepted = true;
    rms_->OnOrderAccepted(&order);

    LOG_INFO(
        "[OMS::OnOrderAccepted] order accepted. OrderID:{}, {}, {}{}, {}, Volume:{}, Price:{:.3f}",
        rsp.order_id, order.req.contract->ticker, ToString(order.req.direction),
        ToString(order.req.offset), ToString(order.req.type), order.req.volume, order.req.price);
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
    LOG_INFO("[OMS::OnPrimaryMarketTraded] done. {}, {}, Volume:{}", order.req.contract->ticker,
             ToString(order.req.direction), order.req.volume);
    order_map_.erase(iter);
  }
}

void OrderManagementSystem::OnSecondaryMarketTraded(const Trade& rsp) {
  std::unique_lock<SpinLock> lock(spinlock_);
  auto iter = order_map_.find(rsp.order_id);
  if (iter == order_map_.end()) {
    LOG_WARN("[OMS::OnSecondaryMarketTraded] Order not found. OrderID:{}, Traded:{}, Price:{:.3f}",
             rsp.order_id, rsp.volume, rsp.price);
    return;
  }

  auto& order = iter->second;
  if (!order.accepted) {
    order.accepted = true;
    rms_->OnOrderAccepted(&order);
    SendRspToStrategy(order, 0, 0.0, NO_ERROR);

    LOG_INFO(
        "[OMS::OnOrderAccepted] order accepted. OrderID:{}, {}, {}{}, {}, Volume:{}, Price:{:.3f}",
        rsp.order_id, order.req.contract->ticker, ToString(order.req.direction),
        ToString(order.req.offset), ToString(order.req.type), order.req.volume, order.req.price);
  }

  order.traded_volume += rsp.volume;

  LOG_INFO(
      "[OMS::OnOrderTraded] order traded. OrderID: {}, {}, {}{}, Traded:{}, Price:{:.3f}, "
      "TotalTraded/Original:{}/{}",
      rsp.order_id, order.req.contract->ticker, ToString(order.req.direction),
      ToString(order.req.offset), rsp.volume, rsp.price, order.traded_volume, order.req.volume);

  rms_->OnOrderTraded(&order, &rsp);
  SendRspToStrategy(order, rsp.volume, rsp.price, NO_ERROR);

  if (order.traded_volume + order.canceled_volume == order.req.volume) {
    LOG_INFO("[OMS::OnOrderTraded] order completed. OrderID:{}, {}, {}{}, Traded/Original: {}/{}",
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
    LOG_WARN("[OMS::OnOrderCanceled] Order not found. OrderID:{}", rsp.order_id);
    return;
  }

  auto& order = iter->second;
  order.canceled_volume = rsp.canceled_volume;

  LOG_INFO("[OMS::OnOrderCanceled] order canceled. {}, {}{}, OrderID:{}, Canceled:{}",
           order.req.contract->ticker, ToString(order.req.direction), ToString(order.req.offset),
           rsp.order_id, rsp.canceled_volume);

  rms_->OnOrderCanceled(&order, rsp.canceled_volume);
  SendRspToStrategy(order, 0, 0.0, NO_ERROR);

  if (order.traded_volume + order.canceled_volume == order.req.volume) {
    LOG_INFO(
        "[OMS::OnOrderCanceled] order completed. OrderID:{}, {}, {}{}, {}, Traded/Original:{}/{}",
        rsp.order_id, order.req.contract->ticker, ToString(order.req.direction),
        ToString(order.req.offset), ToString(order.req.type), order.traded_volume,
        order.req.volume);

    rms_->OnOrderCompleted(&order);
    order_map_.erase(iter);
  }
}

void OrderManagementSystem::operator()(const OrderCancelRejection& rsp) {
  LOG_WARN("[OMS::OnOrderCancelRejected] order cannot be canceled: {}. OrderID:{}", rsp.reason,
           rsp.order_id);
}

}  // namespace ft
