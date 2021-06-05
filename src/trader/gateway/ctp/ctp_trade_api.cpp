// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "trader/gateway/ctp/ctp_trade_api.h"

#include <algorithm>
#include <thread>

#include "ThostFtdcTraderApi.h"
#include "ft/base/log.h"
#include "ft/utils/misc.h"
#include "ft/utils/protocol_utils.h"
#include "trader/gateway/ctp/ctp_gateway.h"

namespace ft {

CtpTradeApi::CtpTradeApi(CtpGateway *gateway)
    : gateway_(gateway), trade_api_(CThostFtdcTraderApi::CreateFtdcTraderApi()) {
  if (!trade_api_) {
    LOG_ERROR("[CtpTradeApi::CtpTradeApi] Failed to CreateFtdcTraderApi");
    exit(-1);
  }
}

CtpTradeApi::~CtpTradeApi() {
  if (trade_api_) {
    Logout();
    trade_api_->Join();
  }
}

bool CtpTradeApi::Login(const GatewayConfig &config) {
  config_ = &config;
  front_addr_ = config.trade_server_address;
  broker_id_ = config.broker_id;
  investor_id_ = config.investor_id;

  trade_api_->SubscribePrivateTopic(THOST_TERT_QUICK);
  trade_api_->RegisterSpi(this);
  trade_api_->RegisterFront(const_cast<char *>(config.trade_server_address.c_str()));
  trade_api_->Init();

  std::this_thread::sleep_for(std::chrono::seconds(1));

  while (status_.load(std::memory_order::memory_order_relaxed) == 0) {
    continue;
  }
  if (status_.load(std::memory_order::memory_order_acquire) != 1) {
    return false;
  }

  if (config_->cancel_outstanding_orders_on_startup) {
    LOG_DEBUG("[CtpTradeApi::Login] Cancel outstanding orders on startup");

    std::this_thread::sleep_for(std::chrono::seconds(1));
    CThostFtdcQryOrderField req{};
    strncpy(req.BrokerID, broker_id_.c_str(), sizeof(req.BrokerID));
    strncpy(req.InvestorID, investor_id_.c_str(), sizeof(req.InvestorID));

    if (trade_api_->ReqQryOrder(&req, next_req_id()) != 0) {
      LOG_ERROR("[CtpTradeApi::Login] Failed. Failed to ReqQryOrder");
      return false;
    }
  }
  return true;
}

void CtpTradeApi::Logout() {
  CThostFtdcUserLogoutField req{};
  strncpy(req.BrokerID, broker_id_.c_str(), sizeof(req.BrokerID));
  strncpy(req.UserID, investor_id_.c_str(), sizeof(req.UserID));
  if (trade_api_->ReqUserLogout(&req, next_req_id()) != 0) {
    return;
  }
}

void CtpTradeApi::OnFrontConnected() {
  LOG_DEBUG("[CtpTradeApi::OnFrontConnected] Success. Connected to {}", front_addr_);

  CThostFtdcReqAuthenticateField auth_req{};
  strncpy(auth_req.BrokerID, config_->broker_id.c_str(), sizeof(auth_req.BrokerID));
  strncpy(auth_req.UserID, config_->investor_id.c_str(), sizeof(auth_req.UserID));
  strncpy(auth_req.AuthCode, config_->auth_code.c_str(), sizeof(auth_req.AuthCode));
  strncpy(auth_req.AppID, config_->app_id.c_str(), sizeof(auth_req.AppID));

  if (trade_api_->ReqAuthenticate(&auth_req, next_req_id()) != 0) {
    LOG_ERROR("[CtpTradeApi::Login] Failed. Failed to ReqAuthenticate");
    status_.store(-1, std::memory_order::memory_order_release);
  }
}

void CtpTradeApi::OnFrontDisconnected(int reason) {
  LOG_ERROR("[CtpTradeApi::OnFrontDisconnected] . Disconnected from {}", front_addr_);
  status_.store(0, std::memory_order::memory_order_relaxed);
}

void CtpTradeApi::OnHeartBeatWarning(int time_lapse) {
  LOG_WARN("[CtpTradeApi::OnHeartBeatWarning] No packet received for some time");
}

void CtpTradeApi::OnRspAuthenticate(CThostFtdcRspAuthenticateField *rsp_authenticate_field,
                                    CThostFtdcRspInfoField *rsp_info, int req_id, bool is_last) {
  if (!is_last) {
    return;
  }

  if (is_error_rsp(rsp_info)) {
    LOG_ERROR("[CtpTradeApi::OnRspAuthenticate] Failed. ErrorMsg: {}",
              gb2312_to_utf8(rsp_info->ErrorMsg));
    status_.store(-1, std::memory_order::memory_order_release);
    return;
  }

  LOG_DEBUG("[CTP::OnRspAuthenticate] Success. Investor ID: {}", investor_id_);

  CThostFtdcReqUserLoginField login_req{};
  strncpy(login_req.BrokerID, config_->broker_id.c_str(), sizeof(login_req.BrokerID));
  strncpy(login_req.UserID, config_->investor_id.c_str(), sizeof(login_req.UserID));
  strncpy(login_req.Password, config_->password.c_str(), sizeof(login_req.Password));

  if (trade_api_->ReqUserLogin(&login_req, next_req_id()) != 0) {
    LOG_ERROR("[CtpTradeApi::Login] Failed. Failed to ReqUserLogin");
    status_.store(-1, std::memory_order::memory_order_release);
  }
}

void CtpTradeApi::OnRspUserLogin(CThostFtdcRspUserLoginField *rsp_user_login,
                                 CThostFtdcRspInfoField *rsp_info, int req_id, bool is_last) {
  if (!is_last) {
    return;
  }

  if (is_error_rsp(rsp_info)) {
    LOG_ERROR("[CtpTradeApi::OnRspUserLogin] Failed. ErrorMsg: {}",
              gb2312_to_utf8(rsp_info->ErrorMsg));
    status_.store(-1, std::memory_order::memory_order_release);
    return;
  }

  front_id_ = rsp_user_login->FrontID;
  session_id_ = rsp_user_login->SessionID;
  auto max_order_ref = std::stoul(rsp_user_login->MaxOrderRef);
  order_ref_base_ = max_order_ref + 1;

  LOG_DEBUG(
      "[CtpTradeApi::OnRspUserLogin] Success. Login as {}. "
      "Front ID: {}, Session ID: {}, Max OrderRef: {}",
      investor_id_, front_id_, session_id_, max_order_ref);

  CThostFtdcQrySettlementInfoField settlement_req{};
  strncpy(settlement_req.BrokerID, broker_id_.c_str(), sizeof(settlement_req.BrokerID));
  strncpy(settlement_req.InvestorID, investor_id_.c_str(), sizeof(settlement_req.InvestorID));

  if (trade_api_->ReqQrySettlementInfo(&settlement_req, next_req_id()) != 0) {
    LOG_ERROR("[CtpTradeApi::Login] Failed. Failed to ReqQrySettlementInfo");
    status_.store(-1, std::memory_order::memory_order_release);
  }
}

void CtpTradeApi::OnRspQrySettlementInfo(CThostFtdcSettlementInfoField *settlement_info,
                                         CThostFtdcRspInfoField *rsp_info, int req_id,
                                         bool is_last) {
  if (!is_last) {
    return;
  }

  if (is_error_rsp(rsp_info)) {
    LOG_ERROR("[CTP::OnRspQrySettlementInfo] Failed. ErrorMsg: {}",
              gb2312_to_utf8(rsp_info->ErrorMsg));
    status_.store(-1, std::memory_order::memory_order_release);
    return;
  }

  LOG_DEBUG("[CTP::OnRspQrySettlementInfo] Success");

  CThostFtdcSettlementInfoConfirmField confirm_req{};
  strncpy(confirm_req.BrokerID, broker_id_.c_str(), sizeof(confirm_req.BrokerID));
  strncpy(confirm_req.InvestorID, investor_id_.c_str(), sizeof(confirm_req.InvestorID));

  if (trade_api_->ReqSettlementInfoConfirm(&confirm_req, next_req_id()) != 0) {
    LOG_ERROR("[CtpTradeApi::Login] Failed. Failed to ReqSettlementInfoConfirm");
    status_.store(-1, std::memory_order::memory_order_release);
  }
}

void CtpTradeApi::OnRspSettlementInfoConfirm(
    CThostFtdcSettlementInfoConfirmField *settlement_info_confirm, CThostFtdcRspInfoField *rsp_info,
    int req_id, bool is_last) {
  if (!is_last) return;

  if (is_error_rsp(rsp_info)) {
    LOG_DEBUG("[CtpTradeApi::OnRspSettlementInfoConfirm] Failed. ErrorMsg: {}",
              gb2312_to_utf8(rsp_info->ErrorMsg));
    status_.store(-1, std::memory_order::memory_order_release);
    return;
  }

  LOG_DEBUG(
      "[CtpTradeApi::OnRspSettlementInfoConfirm] Success. Settlement "
      "confirmed");
  status_.store(1, std::memory_order::memory_order_release);
}

void CtpTradeApi::OnRspUserLogout(CThostFtdcUserLogoutField *user_logout,
                                  CThostFtdcRspInfoField *rsp_info, int req_id, bool is_last) {
  LOG_DEBUG("[CtpTradeApi::OnRspUserLogout] Success. Broker ID: {}, Investor ID: {}",
            user_logout->BrokerID, user_logout->UserID);
  status_.store(0, std::memory_order::memory_order_relaxed);
}

bool CtpTradeApi::SendOrder(const OrderRequest &order, uint64_t *privdata_ptr) {
  auto contract = order.contract;

  auto order_ref = get_order_ref(order.order_id);
  CThostFtdcInputOrderField req{};
  strncpy(req.BrokerID, broker_id_.c_str(), sizeof(req.BrokerID));
  strncpy(req.InvestorID, investor_id_.c_str(), sizeof(req.InvestorID));
  strncpy(req.InstrumentID, contract->ticker.c_str(), sizeof(req.InstrumentID));
  strncpy(req.ExchangeID, contract->exchange.c_str(), sizeof(req.ExchangeID));
  snprintf(req.OrderRef, sizeof(req.OrderRef), "%lu", order_ref);
  req.OrderPriceType = order_type(order.type);
  req.Direction = direction(order.direction);
  req.CombOffsetFlag[0] = offset(order.offset);
  req.LimitPrice = order.price;
  req.VolumeTotalOriginal = order.volume;
  req.CombHedgeFlag[0] =
      order.flags & OrderFlag::kHedge ? THOST_FTDC_HF_Hedge : THOST_FTDC_HF_Speculation;
  req.ContingentCondition = THOST_FTDC_CC_Immediately;
  req.ForceCloseReason = THOST_FTDC_FCC_NotForceClose;
  req.MinVolume = 1;
  req.IsAutoSuspend = 0;
  req.UserForceClose = 0;

  if (order.type == OrderType::kFak) {
    req.TimeCondition = THOST_FTDC_TC_IOC;
    req.VolumeCondition = THOST_FTDC_VC_AV;
  } else if (order.type == OrderType::kFok) {
    req.TimeCondition = THOST_FTDC_TC_IOC;
    req.VolumeCondition = THOST_FTDC_VC_CV;
  } else if (order.type == OrderType::kLimit) {
    req.TimeCondition = THOST_FTDC_TC_GFD;
    req.VolumeCondition = THOST_FTDC_VC_AV;
  } else if (order.type == OrderType::kMarket) {
    req.TimeCondition = THOST_FTDC_TC_IOC;
    req.VolumeCondition = THOST_FTDC_VC_AV;
    req.LimitPrice = 0.0;
  }

  if (trade_api_->ReqOrderInsert(&req, next_req_id()) != 0) {
    LOG_ERROR("[CtpTradeApi::SendOrder] Failed to call ReqOrderInsert");
    return false;
  }

  LOG_DEBUG(
      "[CtpTradeApi::SendOrder] 订单发送成功. OrderID: {}, {}, {}, {}{}"
      "Volume:{}, Price:{:.3f}",
      order.order_id, order_ref, contract->ticker, ToString(order.direction),
      ToString(order.offset), order.volume, order.price);
  *privdata_ptr = static_cast<uint64_t>(order.contract->ticker_id);
  return true;
}

void CtpTradeApi::OnRspOrderInsert(CThostFtdcInputOrderField *order,
                                   CThostFtdcRspInfoField *rsp_info, int req_id, bool is_last) {
  if (!order) {
    LOG_WARN("[CtpTradeApi::OnRspOrderInsert] nullptr");
    return;
  }

  if (order->InvestorID != investor_id_) {
    LOG_WARN(
        "[CtpTradeApi::OnRspOrderInsert] Failed. "
        "= =#Receive RspOrderInsert of other investor");
    return;
  }

  uint64_t order_ref;
  try {
    order_ref = std::stoul(order->OrderRef);
  } catch (...) {
    LOG_ERROR("[CtpTradeApi::OnRspOrderInsert] Invalid order ref");
    return;
  }

  OrderRejectedRsp rsp{get_order_id(order_ref), gb2312_to_utf8(rsp_info->ErrorMsg)};
  gateway_->OnOrderRejected(rsp);
}

void CtpTradeApi::OnRtnOrder(CThostFtdcOrderField *order) {
  if (!order) {
    LOG_WARN("[CtpTradeApi::OnRtnOrder] nullptr");
    return;
  }

  // 过滤不是自己的订单
  if (order->InvestorID != investor_id_) {
    LOG_WARN("[CtpTradeApi::OnRtnOrder] Failed. Unknown order");
    return;
  }

  uint64_t order_id = get_order_id(std::stoul(order->OrderRef));

  // 被拒单或撤销被拒，回调相应函数
  if (order->OrderSubmitStatus == THOST_FTDC_OSS_InsertRejected) {
    OrderRejectedRsp rsp{order_id, gb2312_to_utf8(order->StatusMsg)};
    gateway_->OnOrderRejected(rsp);
    return;
  } else if (order->OrderSubmitStatus == THOST_FTDC_OSS_CancelRejected) {
    OrderCancelRejectedRsp rsp = {order_id, gb2312_to_utf8(order->StatusMsg)};
    gateway_->OnOrderCancelRejected(rsp);
    return;
  } else if ((order->OrderSubmitStatus == THOST_FTDC_OSS_InsertSubmitted &&
              order->OrderStatus != THOST_FTDC_OST_Canceled) ||
             order->OrderSubmitStatus == THOST_FTDC_OSS_CancelSubmitted) {
    return;
  }

  if (order->OrderStatus == THOST_FTDC_OST_Unknown ||
      order->OrderStatus == THOST_FTDC_OST_NoTradeNotQueueing)
    return;

  // 处理撤单
  if (order->OrderStatus == THOST_FTDC_OST_PartTradedNotQueueing ||
      order->OrderStatus == THOST_FTDC_OST_Canceled) {
    OrderCanceledRsp rsp = {order_id, order->VolumeTotalOriginal - order->VolumeTraded};
    gateway_->OnOrderCanceled(rsp);
  } else if (order->OrderStatus == THOST_FTDC_OST_NoTradeQueueing) {
    OrderAcceptedRsp rsp = {order_id};
    gateway_->OnOrderAccepted(rsp);
  }
}

void CtpTradeApi::OnRtnTrade(CThostFtdcTradeField *trade) {
  if (!trade) {
    LOG_WARN("[CtpTradeApi::OnRtnTrade] nullptr");
    return;
  }

  if (trade->InvestorID != investor_id_) {
    LOG_WARN("[CtpTradeApi::OnRtnTrade] Failed. Recv unknown trade");
    return;
  }

  dt_converter_.UpdateDate(trade->TradeDate);

  OrderTradedRsp rsp{};
  rsp.order_id = get_order_id(std::stoul(trade->OrderRef));
  rsp.volume = trade->Volume;
  rsp.price = trade->Price;
  rsp.timestamp_us = dt_converter_.GetExchTimeStamp(trade->TradeTime, 0);
  gateway_->OnOrderTraded(rsp);
}

bool CtpTradeApi::CancelOrder(uint64_t order_id, uint64_t ticker_id) {
  auto contract = ContractTable::get_by_index(ticker_id);
  assert(contract);

  CThostFtdcInputOrderActionField req{};
  req.SessionID = session_id_;
  req.FrontID = front_id_;
  strncpy(req.BrokerID, broker_id_.c_str(), sizeof(req.BrokerID));
  strncpy(req.InvestorID, investor_id_.c_str(), sizeof(req.InvestorID));
  strncpy(req.InstrumentID, contract->ticker.c_str(), sizeof(req.InstrumentID));
  strncpy(req.ExchangeID, contract->exchange.c_str(), sizeof(req.ExchangeID));
  snprintf(req.OrderRef, sizeof(req.OrderRef), "%lu", get_order_ref(order_id));
  req.ActionFlag = THOST_FTDC_AF_Delete;

  if (trade_api_->ReqOrderAction(&req, next_req_id()) != 0) {
    LOG_ERROR("[CtpTradeApi::CancelOrder] Failed to ReqOrderAction");
    return false;
  }

  return true;
}

void CtpTradeApi::OnRspOrderAction(CThostFtdcInputOrderActionField *action,
                                   CThostFtdcRspInfoField *rsp_info, int req_id, bool is_last) {
  if (!action) LOG_WARN("[CtpTradeApi::OnRspOrderAction] nullptr");

  if (action->InvestorID != investor_id_) {
    LOG_WARN("[CtpTradeApi::OnRspOrderAction] Recv unknown action");
    return;
  }

  LOG_ERROR("[CtpTradeApi::OnRspOrderAction] Rejected. Reason: {}",
            gb2312_to_utf8(rsp_info->ErrorMsg));
}

bool CtpTradeApi::QueryContracts() {
  CThostFtdcQryInstrumentField req{};
  if (trade_api_->ReqQryInstrument(&req, next_req_id()) != 0) {
    LOG_ERROR("[CtpTradeApi::QueryContract] Failed to ReqQryInstrument");
    return false;
  }

  return true;
}

void CtpTradeApi::OnRspQryInstrument(CThostFtdcInstrumentField *instrument,
                                     CThostFtdcRspInfoField *rsp_info, int req_id, bool is_last) {
  if (is_error_rsp(rsp_info)) {
    LOG_ERROR("[CtpTradeApi::OnRspQryInstrument] Error Msg: {}",
              gb2312_to_utf8(rsp_info->ErrorMsg));
    gateway_->OnQueryContractEnd();
    return;
  }

  if (instrument) {
    LOG_DEBUG("[CtpTradeApi::OnRspQryInstrument] Instrument: {}, Exchange: {}, {}",
              instrument->InstrumentID, instrument->ExchangeID, instrument->LongMarginRatio);

    Contract contract;
    contract.product_type = product_type(instrument->ProductClass);
    contract.ticker = instrument->InstrumentID;
    contract.exchange = instrument->ExchangeID;
    contract.name = gb2312_to_utf8(instrument->InstrumentName);
    contract.product_type = product_type(instrument->ProductClass);
    contract.size = instrument->VolumeMultiple;
    contract.price_tick = instrument->PriceTick;
    contract.long_margin_rate = instrument->LongMarginRatio;
    contract.short_margin_rate = instrument->ShortMarginRatio;
    contract.max_market_order_volume = instrument->MaxMarketOrderVolume;
    contract.min_market_order_volume = instrument->MinMarketOrderVolume;
    contract.max_limit_order_volume = instrument->MaxLimitOrderVolume;
    contract.min_limit_order_volume = instrument->MinLimitOrderVolume;
    contract.delivery_year = instrument->DeliveryYear;
    contract.delivery_month = instrument->DeliveryMonth;

    gateway_->OnQueryContract(contract);
  }

  if (is_last) {
    gateway_->OnQueryContractEnd();
  }
}

bool CtpTradeApi::QueryPositions() {
  CThostFtdcQryInvestorPositionField req{};
  strncpy(req.BrokerID, broker_id_.c_str(), sizeof(req.BrokerID));
  strncpy(req.InvestorID, investor_id_.c_str(), sizeof(req.InvestorID));

  if (trade_api_->ReqQryInvestorPosition(&req, next_req_id()) != 0) {
    LOG_ERROR("[CtpTradeApi::QueryPosition] Failed to send query req");
    return false;
  }

  return true;
}

void CtpTradeApi::OnRspQryInvestorPosition(CThostFtdcInvestorPositionField *position,
                                           CThostFtdcRspInfoField *rsp_info, int req_id,
                                           bool is_last) {
  if (is_error_rsp(rsp_info)) {
    LOG_ERROR("[CtpTradeApi::OnRspQryInvestorPosition] Error Msg: {}",
              gb2312_to_utf8(rsp_info->ErrorMsg));
    pos_cache_.clear();
    gateway_->OnQueryPositionEnd();
    return;
  }

  if (position) {
    auto contract = ContractTable::get_by_ticker(position->InstrumentID);
    if (!contract) {
      LOG_ERROR("[CtpTradeApi::OnRspQryInvestorPosition] Contract not found: {}",
                position->InstrumentID);
      goto check_last;
    }

    auto &pos = pos_cache_[contract->ticker_id];
    pos.ticker_id = contract->ticker_id;

    bool is_long_pos = position->PosiDirection == THOST_FTDC_PD_Long;
    auto &pos_detail = is_long_pos ? pos.long_pos : pos.short_pos;
    pos_detail.holdings = position->Position;
    pos_detail.yd_holdings = position->Position - position->TodayPosition;
    pos_detail.float_pnl = position->PositionProfit;

    if (is_long_pos)
      pos_detail.frozen = position->LongFrozen;
    else
      pos_detail.frozen = position->ShortFrozen;

    if (pos_detail.holdings > 0 && contract->size > 0)
      pos_detail.cost_price = position->PositionCost / (pos_detail.holdings * contract->size);

    LOG_DEBUG(
        "[CtpTradeApi::OnRspQryInvestorPosition] {}, long:{}, ydlong:{}, "
        "short:{}, ydshort:{}",
        contract->ticker, static_cast<int>(pos.long_pos.holdings),
        static_cast<int>(pos.long_pos.yd_holdings), static_cast<int>(pos.short_pos.holdings),
        static_cast<int>(pos.short_pos.yd_holdings));
  }

check_last:
  if (is_last) {
    for (auto &[ticker_id, pos] : pos_cache_) {
      gateway_->OnQueryPosition(pos);
    }
    gateway_->OnQueryPositionEnd();
    pos_cache_.clear();
  }
}

bool CtpTradeApi::QueryAccount() {
  CThostFtdcQryTradingAccountField req{};
  strncpy(req.BrokerID, broker_id_.c_str(), sizeof(req.BrokerID));
  strncpy(req.InvestorID, investor_id_.c_str(), sizeof(req.InvestorID));

  if (trade_api_->ReqQryTradingAccount(&req, next_req_id()) != 0) {
    LOG_ERROR("[CtpTradeApi::QueryAccount] Failed to ReqQryTradingAccount");
    return false;
  }

  return true;
}

void CtpTradeApi::OnRspQryTradingAccount(CThostFtdcTradingAccountField *trading_account,
                                         CThostFtdcRspInfoField *rsp_info, int req_id,
                                         bool is_last) {
  if (!is_last) return;

  if (is_error_rsp(rsp_info)) {
    LOG_ERROR("[CtpTradeApi::OnRspQryTradingAccount] ErrorMsg: {}",
              gb2312_to_utf8(rsp_info->ErrorMsg));
    gateway_->OnQueryAccountEnd();
    return;
  }

  LOG_DEBUG(
      "[CtpTradeApi::OnRspQryTradingAccount] Account ID: {}, Balance: {:.3f}, "
      "Frozen: {:.3f}, Margin: {:.3f}",
      trading_account->AccountID, trading_account->Balance, trading_account->FrozenMargin,
      trading_account->CurrMargin);

  Account account;
  account.account_id = std::stoul(trading_account->AccountID);
  account.total_asset = trading_account->Balance;
  account.margin = trading_account->CurrMargin;
  account.frozen = trading_account->FrozenCash + trading_account->FrozenMargin +
                   trading_account->FrozenCommission;
  account.cash = account.total_asset - account.margin - account.frozen;

  gateway_->OnQueryAccount(account);
  gateway_->OnQueryAccountEnd();
}

void CtpTradeApi::OnRspQryOrder(CThostFtdcOrderField *order, CThostFtdcRspInfoField *rsp_info,
                                int req_id, bool is_last) {
  if (is_error_rsp(rsp_info)) {
    LOG_ERROR("[CtpTradeApi::OnRspQryOrder] ErrorMsg: {}", gb2312_to_utf8(rsp_info->ErrorMsg));
    status_.store(-1, std::memory_order::memory_order_release);
    return;
  }

  if (order && (order->OrderStatus == THOST_FTDC_OST_NoTradeQueueing ||
                order->OrderStatus == THOST_FTDC_OST_PartTradedQueueing)) {
    LOG_INFO(
        "[CtpTradeApi::OnRspQryOrder] Cancel all orders on startup. Ticker: "
        "{}.{}, OrderSysID: {}, OriginalVolume: {}, Traded: {}, StatusMsg: {}",
        order->InstrumentID, order->ExchangeID, order->OrderSysID, order->VolumeTotalOriginal,
        order->VolumeTraded, gb2312_to_utf8(order->StatusMsg));

    CThostFtdcInputOrderActionField req{};
    strncpy(req.ExchangeID, order->ExchangeID, sizeof(req.ExchangeID));
    strncpy(req.OrderSysID, order->OrderSysID, sizeof(req.OrderSysID));
    strncpy(req.BrokerID, broker_id_.c_str(), sizeof(req.BrokerID));
    strncpy(req.InvestorID, investor_id_.c_str(), sizeof(req.InvestorID));
    req.ActionFlag = THOST_FTDC_AF_Delete;

    if (trade_api_->ReqOrderAction(&req, next_req_id()) != 0)
      LOG_ERROR("[CtpTradeApi::OnRspQryOrder] Failed to call ReqOrderAction");
  }

  if (is_last) {
    status_.store(1, std::memory_order::memory_order_release);
  }
}

bool CtpTradeApi::QueryTrades() {
  CThostFtdcQryTradeField req{};
  strncpy(req.BrokerID, broker_id_.c_str(), sizeof(req.BrokerID));
  strncpy(req.InvestorID, investor_id_.c_str(), sizeof(req.InvestorID));

  if (trade_api_->ReqQryTrade(&req, next_req_id()) != 0) {
    LOG_ERROR("[CtpTradeApi::QueryTradeList] Failed. Failed to ReqQryTrade");
    return false;
  }

  return true;
}

void CtpTradeApi::OnRspQryTrade(CThostFtdcTradeField *trade, CThostFtdcRspInfoField *rsp_info,
                                int req_id, bool is_last) {
  if (is_error_rsp(rsp_info)) {
    LOG_ERROR("[CtpTradeApi::OnRspQryTrade] Failed. ErrorMsg: {}",
              gb2312_to_utf8(rsp_info->ErrorMsg));
    gateway_->OnQueryContractEnd();
    return;
  }

  if (trade) {
    auto contract = ContractTable::get_by_ticker(trade->InstrumentID);
    assert(contract);

    HistoricalTrade td{};
    td.order_sys_id = std::stoul(trade->OrderSysID);
    td.ticker_id = contract->ticker_id;
    td.volume = trade->Volume;
    td.price = trade->Price;
    td.direction = direction(trade->Direction);
    td.offset = offset(trade->OffsetFlag);

    gateway_->OnQueryTrade(td);
  }

  if (is_last) {
    gateway_->OnQueryTradeEnd();
  }
}

}  // namespace ft
