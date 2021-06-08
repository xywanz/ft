// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "trader/oms.h"

#include <dlfcn.h>

#include <utility>

#include "ft/base/contract_table.h"
#include "ft/base/log.h"
#include "ft/component/yijinjing/journal/Timer.h"
#include "ft/utils/misc.h"
#include "ft/utils/protocol_utils.h"

namespace ft {

OrderManagementSystem::OrderManagementSystem() { rms_ = std::make_unique<RiskManagementSystem>(); }

bool OrderManagementSystem::Init(const FlareTraderConfig& config) {
  LOG_INFO("OMS compiling time: {} {}", __TIME__, __DATE__);

  config_ = &config;

  if (!InitContractTable()) {
    return false;
  }

  if (!InitTraderDBConn()) {
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

  tick_thread_ = std::thread(std::mem_fn(&OrderManagementSystem::ProcessTick), this);

  LOG_INFO("ft_trader inited");
  is_logon_ = true;

  return true;
}

void OrderManagementSystem::Run() {
  for (;;) {
    ProcessCmd();
    ProcessRsp();
  }
}

void OrderManagementSystem::ProcessCmd() {
  yijinjing::FramePtr frame;
  for (std::size_t i = 0; i < trade_msg_readers_.size(); ++i) {
    auto& reader = trade_msg_readers_[i];
    while ((frame = reader->getNextFrame()) != nullptr) {
      if (frame->getDataLength() != sizeof(TraderCommand)) {
        LOG_ERROR("[OMS::ProcessCmd] invalid trader cmd size");
        continue;
      }
      auto* cmd = reinterpret_cast<TraderCommand*>(frame->getData());
      ExecuteCmd(*cmd, static_cast<uint32_t>(i));
    }
  }
}

void OrderManagementSystem::ProcessRsp() {
  int count = 0;
  auto* rsp_rb = gateway_->GetOrderRspRB();
  GatewayOrderResponse rsp;
  while (count < 3 && rsp_rb->Get(&rsp)) {
    std::visit(*this, rsp.data);
    ++count;
  }
}

void OrderManagementSystem::ProcessTick() {
  auto* tick_rb = gateway_->GetTickRB();
  TickData tick;
  for (;;) {
    tick_rb->GetWithBlocking(&tick);
    OnTick(tick);
  }
}

void OrderManagementSystem::ExecuteCmd(const TraderCommand& cmd, uint32_t mq_id) {
  if (cmd.magic != kTradingCmdMagic) {
    LOG_ERROR("[OMS::ExecuteCmd] invalid magic number of cmd");
    return;
  }

  switch (cmd.type) {
    case TraderCmdType::kNewOrder: {
      SendOrder(cmd, mq_id);
      break;
    }
    case TraderCmdType::kCancelOrder: {
      CancelOrder(cmd.cancel_req.order_id, cmd.without_check);
      break;
    }
    case TraderCmdType::kCancelTicker: {
      CancelForTicker(cmd.cancel_ticker_req.ticker_id, cmd.without_check);
      break;
    }
    case TraderCmdType::kCancelAll: {
      CancelAll(cmd.without_check);
      break;
    }
    case TraderCmdType::kNotify: {
      gateway_->OnNotify(cmd.notification.signal);
      break;
    }
    default: {
      LOG_ERROR("[OMS::ExecuteCmd] unknown cmd");
      break;
    }
  }
}

bool OrderManagementSystem::SendOrder(const TraderCommand& cmd, uint32_t mq_id) {
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
  order.mq_id = mq_id;
  order.status = OrderStatus::kSubmitting;
  order.strategy_id = cmd.strategy_id;

  std::unique_lock<SpinLock> lock(spinlock_);
  // 增加是否经过风控检查字段，在紧急情况下可以设置该字段绕过风控下单
  if (!cmd.without_check) {
    auto error_code = rms_->CheckOrderRequest(order);
    if (error_code != ErrorCode::kNoError) {
      LOG_ERROR("[OMS::SendOrder] risk: {}", ErrorCodeStr(error_code));
      SendRspToStrategy(order, 0, 0.0, error_code);
      return false;
    }
  }

#ifdef FT_MEASURE_TICK_TO_TRADE
  timespec ts;
  clock_gettime(CLOCK_REALTIME, &ts);
  LOG_INFO("tick-to-trade: {} us", ts.tv_sec * 1000000UL + ts.tv_nsec / 1000UL - cmd.timestamp_us);
#endif

  if (!gateway_->SendOrder(req, &order.privdata)) {
    LOG_ERROR("[OMS::SendOrder] failed to send order. {}, {}{}, {}, Volume:{}, Price:{:.3f}",
              contract->ticker, ToString(req.direction), ToString(req.offset), ToString(req.type),
              req.volume, req.price);

    rms_->OnOrderRejected(order, ErrorCode::kSendFailed);
    SendRspToStrategy(order, 0, 0.0, ErrorCode::kSendFailed);
    return false;
  }

  order_map_.emplace(req.order_id, order);
  rms_->OnOrderSent(order);

  LOG_DEBUG("[OMS::SendOrder] success. OrderID:{}, {}, {}{}, {}, Volume:{}, Price:{:.3f}",
            req.order_id, contract->ticker, ToString(req.direction), ToString(req.offset),
            ToString(req.type), req.volume, req.price);
  return true;
}

void OrderManagementSystem::DoCancelOrder(const Order& order, bool without_check) {
  if (!without_check) {
    auto error_code = rms_->CheckCancelReq(order);
    if (error_code != ErrorCode::kNoError) {
      LOG_ERROR("[OMS::DoCancelOrder] risk: {}", ErrorCodeStr(error_code));
      return;
    }
  }
  if (!gateway_->CancelOrder(order.req.order_id, order.privdata)) {
    LOG_ERROR("[OMS::DoCancelOrder] error occurred in Gateway::CancelOrder");
    return;
  }
}

void OrderManagementSystem::CancelOrder(uint64_t order_id, bool without_check) {
  std::unique_lock<SpinLock> lock(spinlock_);
  auto iter = order_map_.find(order_id);
  if (iter == order_map_.end()) {
    LOG_ERROR("[OMS::CancelOrder] order not found. order_id:{}", order_id);
    return;
  }
  DoCancelOrder(iter->second, without_check);
}

void OrderManagementSystem::CancelForTicker(uint32_t ticker_id, bool without_check) {
  std::unique_lock<SpinLock> lock(spinlock_);
  for (const auto& [order_id, order] : order_map_) {
    (void)order_id;
    if (ticker_id == order.req.contract->ticker_id) {
      DoCancelOrder(order, without_check);
    }
  }
}

void OrderManagementSystem::CancelAll(bool without_check) {
  std::unique_lock<SpinLock> lock(spinlock_);
  for (const auto& [order_id, order] : order_map_) {
    (void)order_id;
    DoCancelOrder(order, without_check);
  }
}

bool OrderManagementSystem::InitTraderDBConn() {
  if (!trader_db_updater_.Init(config_->global_config.trader_db_address, "", "")) {
    LOG_ERROR("[OMS::InitTraderDBConn] failed");
    return false;
  }
  return true;
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
  pos_manager_.Init(*config_, [this](const std::string& strategy, const Position& new_pos) {
    auto* contract = ContractTable::get_by_index(new_pos.ticker_id);
    if (!contract) {
      LOG_ERROR(
          "[OMS::UpdatePosition] contract not found. failed to update positions in redis. "
          "ticker_id:{}",
          new_pos.ticker_id);
      return;
    }
    if (!trader_db_updater_.SetPosition(strategy, contract->ticker, new_pos)) {
      LOG_ERROR("[OMS::UpdatePosition] failed");
      // TODO: 异常处理
    }
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
  if (!OnPositions(&init_positions)) {
    return false;
  }
  return true;
}

bool OrderManagementSystem::InitTradeInfo() {
  auto qry_res_rb = gateway_->GetQryResultRB();
  GatewayQueryResult qry_res;

  // query trades to update position
  std::vector<HistoricalTrade> init_trades;
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
    init_trades.emplace_back(std::get<HistoricalTrade>(qry_res.data));
  }
  OnTrades(&init_trades);
  return true;
}

bool OrderManagementSystem::InitRMS() {
  for (auto& risk_conf : config_->rms_config.risk_conf_list) {
    if (!rms_->AddRule(risk_conf.name)) {
      LOG_ERROR("unknown risk rule: {}", risk_conf.name);
      return false;
    }
  }

  RiskRuleParams risk_params{};
  risk_params.config = &config_->rms_config;
  risk_params.account = &account_;
  risk_params.pos_manager = &pos_manager_;
  risk_params.order_map = &order_map_;
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
                                              ErrorCode error_code) {
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
  rsp.completed = order.canceled_volume + order.traded_volume == order.req.volume ||
                  error_code != ErrorCode::kNoError;
  rsp.error_code = error_code;

  rsp_writers_[order.mq_id]->write_data(rsp, 0, 0);
}

void OrderManagementSystem::OnAccount(const Account& account) {
  std::unique_lock<SpinLock> lock(spinlock_);
  account_ = account;
  lock.unlock();

  LOG_DEBUG("[OMS::OnAccount] account_id:{} total_asset:{} cash:{} margin:{} frozen:{}",
            account.account_id, account.total_asset, account.cash, account.margin, account.frozen);
}

bool OrderManagementSystem::OnPositions(std::vector<Position>* positions) {
  auto* trader_db = trader_db_updater_.GetTraderDB();
  trader_db->ClearPositions(PositionManager::kCommonPosPool);

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

    if (lp.holdings == 0 && lp.frozen == 0 && sp.holdings == 0 && sp.frozen == 0) {
      continue;
    }

    if (!pos_manager_.SetPosition(PositionManager::kCommonPosPool, position)) {
      LOG_ERROR("SetPostion failed");
      return false;
    }
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(100));  // 确保仓位已被写入数据库
  return RecoveryStrategyPositions();
}

bool OrderManagementSystem::RecoveryStrategyPositions() {
  auto* trader_db = trader_db_updater_.GetTraderDB();
  for (auto& strategy_conf : config_->strategy_config_list) {
    std::vector<Position> pos_list;
    if (!trader_db->GetAllPositions(strategy_conf.strategy_name, &pos_list)) {
      LOG_ERROR("OMS::RecoveryStrategyPositions. failed to get strategy({}) pos from db",
                strategy_conf.strategy_name);
      return false;
    }
    for (auto& pos : pos_list) {
      if (!pos_manager_.MovePosition(PositionManager::kCommonPosPool, strategy_conf.strategy_name,
                                     pos.ticker_id, Direction::kBuy, pos.long_pos.holdings)) {
        LOG_ERROR("OMS::RecoveryStrategyPositions. failed to recover long pos. {} {} {}",
                  strategy_conf.strategy_name, pos.ticker_id, pos.long_pos.holdings);
        return false;
      }
      if (!pos_manager_.MovePosition(PositionManager::kCommonPosPool, strategy_conf.strategy_name,
                                     pos.ticker_id, Direction::kSell, pos.short_pos.holdings)) {
        LOG_ERROR("OMS::RecoveryStrategyPositions. failed to recover short pos. {} {} {}",
                  strategy_conf.strategy_name, pos.ticker_id, pos.short_pos.holdings);
        return false;
      }
    }
  }
  return true;
}

void OrderManagementSystem::OnTick(const TickData& tick) {
  auto contract = ContractTable::get_by_index(tick.ticker_id);
  assert(contract);

  auto& writers = md_dispatch_map_[contract->ticker_id];
  for (auto& writer : writers) {
    writer->write_data(tick, 0, 0);
  }

  LOG_TRACE("[OMS::OnTick] {}  ask:{:.3f}  bid:{:.3f}", contract->ticker, tick.ask[0], tick.bid[0]);
}

void OrderManagementSystem::OnTrades(std::vector<HistoricalTrade>* trades) {}

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
void OrderManagementSystem::operator()(const OrderAcceptedRsp& rsp) {
  std::unique_lock<SpinLock> lock(spinlock_);
  auto iter = order_map_.find(rsp.order_id);
  if (iter == order_map_.end()) {
    LOG_WARN("[OMS::OnOrderAccepted] order not found. OrderID: {}", rsp.order_id);
    return;
  }

  auto& order = iter->second;
  if (order.accepted) {
    return;
  }

  order.accepted = true;
  order.status = OrderStatus::kAccepted;
  rms_->OnOrderAccepted(order);
  SendRspToStrategy(order, 0, 0.0, ErrorCode::kNoError);

  LOG_INFO(
      "[OMS::OnOrderAccepted] order accepted. OrderID:{}, {}, {}{}, {}, Volume:{}, Price:{:.2f}",
      rsp.order_id, order.req.contract->ticker, ToString(order.req.direction),
      ToString(order.req.offset), ToString(order.req.type), order.req.volume, order.req.price);
}

void OrderManagementSystem::operator()(const OrderRejectedRsp& rsp) {
  std::unique_lock<SpinLock> lock(spinlock_);
  auto iter = order_map_.find(rsp.order_id);
  if (iter == order_map_.end()) {
    LOG_WARN("[OMS::OnOrderRejected] order not found. order_id:{}", rsp.order_id);
    return;
  }

  auto& order = iter->second;
  order.status = OrderStatus::kRejected;
  rms_->OnOrderRejected(order, ErrorCode::kRejected);
  SendRspToStrategy(order, 0, 0.0, ErrorCode::kRejected);

  LOG_ERROR("[OMS::OnOrderRejected] order rejected. {}. {}, {}{}, {}, Volume:{}, Price:{:.3f}",
            rsp.reason, order.req.contract->ticker, ToString(order.req.direction),
            ToString(order.req.offset), ToString(order.req.type), order.req.volume,
            order.req.price);

  order_map_.erase(iter);
}

void OrderManagementSystem::operator()(const OrderTradedRsp& rsp) { OnSecondaryMarketTraded(rsp); }

void OrderManagementSystem::OnSecondaryMarketTraded(const OrderTradedRsp& rsp) {
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
    rms_->OnOrderAccepted(order);
    SendRspToStrategy(order, 0, 0.0, ErrorCode::kNoError);

    LOG_INFO(
        "[OMS::OnOrderAccepted] order accepted. OrderID:{}, {}, {}{}, {}, Volume:{}, Price:{:.3f}",
        rsp.order_id, order.req.contract->ticker, ToString(order.req.direction),
        ToString(order.req.offset), ToString(order.req.type), order.req.volume, order.req.price);
  }

  order.traded_volume += rsp.volume;
  if (order.traded_volume == order.req.volume) {
    order.status = OrderStatus::kAllTraded;
  } else if (order.status != OrderStatus::kCanceled) {
    order.status = OrderStatus::kPartTraded;
  }

  LOG_INFO(
      "[OMS::OnOrderTraded] order traded. OrderID: {}, {}, {}{}, Traded:{}, Price:{:.3f}, "
      "TotalTraded/Original:{}/{}",
      rsp.order_id, order.req.contract->ticker, ToString(order.req.direction),
      ToString(order.req.offset), rsp.volume, rsp.price, order.traded_volume, order.req.volume);

  rms_->OnOrderTraded(order, rsp);
  SendRspToStrategy(order, rsp.volume, rsp.price, ErrorCode::kNoError);

  if (order.traded_volume + order.canceled_volume == order.req.volume) {
    LOG_INFO("[OMS::OnOrderTraded] order completed. OrderID:{}, {}, {}{}, Traded/Original: {}/{}",
             rsp.order_id, order.req.contract->ticker, ToString(order.req.direction),
             ToString(order.req.offset), order.traded_volume, order.req.volume);

    // 订单结束，通知风控模块
    rms_->OnOrderCompleted(order);
    order_map_.erase(iter);
  }
}

void OrderManagementSystem::operator()(const OrderCanceledRsp& rsp) {
  std::unique_lock<SpinLock> lock(spinlock_);
  auto iter = order_map_.find(rsp.order_id);
  if (iter == order_map_.end()) {
    LOG_WARN("[OMS::OnOrderCanceled] Order not found. OrderID:{}", rsp.order_id);
    return;
  }

  auto& order = iter->second;
  order.canceled_volume = rsp.canceled_volume;
  order.status = OrderStatus::kCanceled;

  LOG_INFO("[OMS::OnOrderCanceled] order canceled. {}, {}{}, OrderID:{}, Canceled:{}",
           order.req.contract->ticker, ToString(order.req.direction), ToString(order.req.offset),
           rsp.order_id, rsp.canceled_volume);

  rms_->OnOrderCanceled(order, rsp.canceled_volume);
  SendRspToStrategy(order, 0, 0.0, ErrorCode::kNoError);

  if (order.traded_volume + order.canceled_volume == order.req.volume) {
    LOG_INFO(
        "[OMS::OnOrderCanceled] order completed. OrderID:{}, {}, {}{}, {}, Traded/Original:{}/{}",
        rsp.order_id, order.req.contract->ticker, ToString(order.req.direction),
        ToString(order.req.offset), ToString(order.req.type), order.traded_volume,
        order.req.volume);

    rms_->OnOrderCompleted(order);
    order_map_.erase(iter);
  }
}

void OrderManagementSystem::operator()(const OrderCancelRejectedRsp& rsp) {
  LOG_WARN("[OMS::OnOrderCancelRejected] order cannot be canceled: {}. OrderID:{}", rsp.reason,
           rsp.order_id);
}

}  // namespace ft
