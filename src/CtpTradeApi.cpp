// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "Api/Ctp/CtpTradeApi.h"

#include <ThostFtdcTraderApi.h>
#include <ThostFtdcUserApiDataType.h>
#include <ThostFtdcUserApiStruct.h>
#include <cppex/split.h>
#include <spdlog/spdlog.h>

#include <cassert>
#include <cstring>

#include "Api/Ctp/CtpCommon.h"
#include "Base/DataStruct.h"
#include "ContractTable.h"

namespace ft {

CtpTradeApi::CtpTradeApi(Gateway *gateway) : gateway_(gateway) {}

CtpTradeApi::~CtpTradeApi() {
  error();
  logout();
}

bool CtpTradeApi::login(const LoginParams &params) {
  if (params.broker_id().size() > sizeof(TThostFtdcBrokerIDType) ||
      params.broker_id().empty() ||
      params.investor_id().size() > sizeof(TThostFtdcUserIDType) ||
      params.investor_id().empty() ||
      params.passwd().size() > sizeof(TThostFtdcPasswordType) ||
      params.passwd().empty() || params.front_addr().empty()) {
    spdlog::error("[CtpTradeApi::login] Failed. Invalid login params");
    return false;
  }

  ctp_api_.reset(CThostFtdcTraderApi::CreateFtdcTraderApi());
  if (!ctp_api_) {
    spdlog::error("[CtpTradeApi::login] Failed. Failed to CreateFtdcTraderApi");
    return false;
  }

  front_addr_ = params.front_addr();
  broker_id_ = params.broker_id();
  investor_id_ = params.investor_id();

  ctp_api_->SubscribePrivateTopic(THOST_TERT_QUICK);
  ctp_api_->RegisterSpi(this);
  ctp_api_->RegisterFront(const_cast<char *>(params.front_addr().c_str()));
  ctp_api_->Init();
  while (!is_connected_) {
    if (is_error_) {
      spdlog::error("[CtpTradeApi::login] Failed. Cannot connect to {}",
                    front_addr_);
      return false;
    }
  }

  if (!params.auth_code().empty()) {
    CThostFtdcReqAuthenticateField auth_req;
    memset(&auth_req, 0, sizeof(auth_req));
    strncpy(auth_req.BrokerID, params.broker_id().c_str(),
            sizeof(auth_req.BrokerID));
    strncpy(auth_req.UserID, params.investor_id().c_str(),
            sizeof(auth_req.UserID));
    strncpy(auth_req.AuthCode, params.auth_code().c_str(),
            sizeof(auth_req.AuthCode));
    strncpy(auth_req.AppID, params.app_id().c_str(), sizeof(auth_req.AppID));

    reset_sync();
    if (ctp_api_->ReqAuthenticate(&auth_req, next_req_id()) != 0) {
      spdlog::error("[CtpTradeApi::login] Failed. Failed to ReqAuthenticate");
      return false;
    }

    if (!wait_sync()) {
      spdlog::error("[CtpTradeApi::login] Failed. Failed to authenticate");
      return false;
    }
  }

  CThostFtdcReqUserLoginField login_req;
  memset(&login_req, 0, sizeof(login_req));
  strncpy(login_req.BrokerID, params.broker_id().c_str(),
          sizeof(login_req.BrokerID));
  strncpy(login_req.UserID, params.investor_id().c_str(),
          sizeof(login_req.UserID));
  strncpy(login_req.Password, params.passwd().c_str(),
          sizeof(login_req.Password));

  reset_sync();
  if (ctp_api_->ReqUserLogin(&login_req, next_req_id()) != 0) {
    spdlog::error("[CtpTradeApi::login] Failed. Failed to ReqUserLogin");
    return false;
  }

  if (!wait_sync()) {
    spdlog::error("[CtpTradeApi::login] Failed. Failed to login");
    return false;
  }

  CThostFtdcQrySettlementInfoField settlement_req;
  memset(&settlement_req, 0, sizeof(settlement_req));
  strncpy(settlement_req.BrokerID, broker_id_.c_str(),
          sizeof(settlement_req.BrokerID));
  strncpy(settlement_req.InvestorID, investor_id_.c_str(),
          sizeof(settlement_req.InvestorID));

  reset_sync();
  if (ctp_api_->ReqQrySettlementInfo(&settlement_req, next_req_id()) != 0) {
    spdlog::error(
        "[CtpTradeApi::login] Failed. Failed to ReqQrySettlementInfo");
    return false;
  }

  if (!wait_sync()) {
    spdlog::error("[CtpTradeApi::login] Failed. Failed to query settlement");
    return false;
  }

  CThostFtdcSettlementInfoConfirmField confirm_req;
  memset(&confirm_req, 0, sizeof(confirm_req));
  strncpy(confirm_req.BrokerID, broker_id_.c_str(),
          sizeof(confirm_req.BrokerID));
  strncpy(confirm_req.InvestorID, investor_id_.c_str(),
          sizeof(confirm_req.InvestorID));

  reset_sync();
  if (ctp_api_->ReqSettlementInfoConfirm(&confirm_req, next_req_id()) != 0) {
    spdlog::error(
        "[CtpTradeApi::login] Failed. Failed to ReqSettlementInfoConfirm");
    return false;
  }

  if (!wait_sync()) {
    spdlog::error(
        "[CtpTradeApi::login] Failed. Failed to confirm settlement info");
    return false;
  }

  is_logon_ = true;

  if (!query_orders()) {
    spdlog::error("[CtpTradeApi::login] Failed. Failed to query_orders");
    return false;
  }

  std::this_thread::sleep_for(std::chrono::seconds(1));
  return true;
}

bool CtpTradeApi::logout() {
  if (is_logon_) {
    CThostFtdcUserLogoutField req;
    strncpy(req.BrokerID, broker_id_.c_str(), sizeof(req.BrokerID));
    strncpy(req.UserID, investor_id_.c_str(), sizeof(req.UserID));
    if (ctp_api_->ReqUserLogout(&req, next_req_id()) != 0) return false;

    while (is_logon_) continue;
  }

  return true;
}

void CtpTradeApi::OnFrontConnected() {
  spdlog::debug("[CtpTradeApi::OnFrontConnected] Success. Connected to {}",
                front_addr_);
  is_error_ = false;
  is_connected_ = true;
}

void CtpTradeApi::OnFrontDisconnected(int reason) {
  spdlog::error("[CtpTradeApi::OnFrontDisconnected] . Disconnected from {}",
                front_addr_);
  error();
  is_connected_ = false;
}

void CtpTradeApi::OnHeartBeatWarning(int time_lapse) {
  spdlog::warn(
      "[CtpTradeApi::OnHeartBeatWarning] Warn. No packet received for a period "
      "of time");
}

void CtpTradeApi::OnRspAuthenticate(
    CThostFtdcRspAuthenticateField *rsp_authenticate_field,
    CThostFtdcRspInfoField *rsp_info, int req_id, bool is_last) {
  if (!is_last) return;

  if (is_error_rsp(rsp_info)) {
    spdlog::error("[CtpTradeApi::OnRspAuthenticate] Failed. ErrorMsg: {}",
                  gb2312_to_utf8(rsp_info->ErrorMsg));
    error();
    return;
  }

  spdlog::debug("[CTP::OnRspAuthenticate] Success. Investor ID: {}",
                investor_id_);
  done();
}

void CtpTradeApi::OnRspUserLogin(CThostFtdcRspUserLoginField *rsp_user_login,
                                 CThostFtdcRspInfoField *rsp_info, int req_id,
                                 bool is_last) {
  if (!is_last) return;

  if (is_error_rsp(rsp_info)) {
    spdlog::error("[CtpTradeApi::OnRspUserLogin] Failed. ErrorMsg: {}",
                  gb2312_to_utf8(rsp_info->ErrorMsg));
    error();
    return;
  }

  front_id_ = rsp_user_login->FrontID;
  session_id_ = rsp_user_login->SessionID;
  int max_order_ref = std::stoi(rsp_user_login->MaxOrderRef);
  next_order_ref_ = max_order_ref + 1;

  spdlog::debug(
      "[CtpTradeApi::OnRspUserLogin] Success. Login as {}. "
      "Front ID: {}, Session ID: {}, Max OrderRef: {}",
      investor_id_, front_id_, session_id_, max_order_ref);
  done();
}

void CtpTradeApi::OnRspQrySettlementInfo(
    CThostFtdcSettlementInfoField *settlement_info,
    CThostFtdcRspInfoField *rsp_info, int req_id, bool is_last) {
  if (!is_last) return;

  if (is_error_rsp(rsp_info)) {
    spdlog::error("[CTP::OnRspQrySettlementInfo] Failed. ErrorMsg: {}",
                  gb2312_to_utf8(rsp_info->ErrorMsg));
    error();
    return;
  }

  spdlog::debug("[CTP::OnRspQrySettlementInfo] Success");
  done();
}

void CtpTradeApi::OnRspSettlementInfoConfirm(
    CThostFtdcSettlementInfoConfirmField *settlement_info_confirm,
    CThostFtdcRspInfoField *rsp_info, int req_id, bool is_last) {
  if (!is_last) return;

  if (is_error_rsp(rsp_info)) {
    spdlog::debug(
        "[CtpTradeApi::OnRspSettlementInfoConfirm] Failed. ErrorMsg: {}",
        gb2312_to_utf8(rsp_info->ErrorMsg));
    error();
    return;
  }

  spdlog::debug(
      "[CtpTradeApi::OnRspSettlementInfoConfirm] Success. Settlement "
      "confirmed");
  done();
}

void CtpTradeApi::OnRspUserLogout(CThostFtdcUserLogoutField *user_logout,
                                  CThostFtdcRspInfoField *rsp_info, int req_id,
                                  bool is_last) {
  spdlog::debug(
      "[CtpTradeApi::OnRspUserLogout] Success. Broker ID: {}, Investor ID: {}",
      user_logout->BrokerID, user_logout->UserID);
  is_logon_ = false;
}

uint64_t CtpTradeApi::send_order(const Order *order) {
  if (!is_logon_) {
    spdlog::error("[CtpTradeApi::send_order] Failed. Not logon");
    return 0;
  }

  const auto *contract = ContractTable::get_by_index(order->ticker_index);
  if (!contract) {
    spdlog::error("[CtpTradeApi::send_order] Contract not found");
    return 0;
  }

  auto order_ref = next_order_ref();
  CThostFtdcInputOrderField ctp_order;
  memset(&ctp_order, 0, sizeof(ctp_order));
  strncpy(ctp_order.BrokerID, broker_id_.c_str(), sizeof(ctp_order.BrokerID));
  strncpy(ctp_order.InvestorID, investor_id_.c_str(),
          sizeof(ctp_order.InvestorID));
  strncpy(ctp_order.InstrumentID, contract->symbol.c_str(),
          sizeof(ctp_order.InstrumentID));
  strncpy(ctp_order.ExchangeID, contract->exchange.c_str(),
          sizeof(ctp_order.ExchangeID));
  snprintf(ctp_order.OrderRef, sizeof(ctp_order.OrderRef), "%d", order_ref);
  ctp_order.OrderPriceType = order_type(order->type);
  ctp_order.Direction = direction(order->direction);
  ctp_order.CombOffsetFlag[0] = offset(order->offset);
  ctp_order.LimitPrice = order->price;
  ctp_order.VolumeTotalOriginal = order->volume;
  ctp_order.CombHedgeFlag[0] = THOST_FTDC_HF_Speculation;
  ctp_order.ContingentCondition = THOST_FTDC_CC_Immediately;
  ctp_order.ForceCloseReason = THOST_FTDC_FCC_NotForceClose;
  ctp_order.MinVolume = 1;
  ctp_order.IsAutoSuspend = 0;
  ctp_order.UserForceClose = 0;

  if (order->type == OrderType::FAK) {
    ctp_order.TimeCondition = THOST_FTDC_TC_IOC;
    ctp_order.VolumeCondition = THOST_FTDC_VC_AV;
  } else if (order->type == OrderType::FOK) {
    ctp_order.TimeCondition = THOST_FTDC_TC_IOC;
    ctp_order.VolumeCondition = THOST_FTDC_VC_CV;
  } else {
    ctp_order.TimeCondition = THOST_FTDC_TC_GFD;
    ctp_order.VolumeCondition = THOST_FTDC_VC_AV;
  }

  if (ctp_api_->ReqOrderInsert(&ctp_order, next_req_id()) != 0) return 0;

  return get_order_id(order->ticker_index, order_ref);
}

void CtpTradeApi::OnRspOrderInsert(CThostFtdcInputOrderField *ctp_order,
                                   CThostFtdcRspInfoField *rsp_info, int req_id,
                                   bool is_last) {
  if (ctp_order->InvestorID != investor_id_) {
    spdlog::error(
        "[CtpTradeApi::OnRspOrderInsert] Failed. "
        "= =#Receive RspOrderInsert of other investor");
    return;
  }

  const auto *contract = ContractTable::get_by_symbol(ctp_order->InstrumentID);
  if (!contract) {
    spdlog::error("[CtpTradeApi::OnRspOrderInsert] Contract not found");
    return;
  }

  Order order;
  order.ticker_index = contract->index;
  order.order_id =
      get_order_id(order.ticker_index, std::stoi(ctp_order->OrderRef));
  order.direction = direction(ctp_order->Direction);
  order.offset = offset(ctp_order->CombOffsetFlag[0]);
  order.price = ctp_order->LimitPrice;
  order.volume = ctp_order->VolumeTotalOriginal;
  order.type = order_type(ctp_order->OrderPriceType);
  order.status = OrderStatus::REJECTED;

  spdlog::error(
      "[CtpTradeApi::OnRspOrderInsert] Rejected. Order ID: {}, Instrument: {}, "
      "Exchange: {}, Direction: {}, Offset: {}, Origin Volume: {}, "
      "Traded: {}, Price: {:.2f}, Status: {}, ErrorMsg: {}",
      order.order_id, contract->symbol, contract->exchange,
      to_string(order.direction), to_string(order.offset), order.volume,
      order.volume_traded, order.price, to_string(order.status),
      gb2312_to_utf8(rsp_info->ErrorMsg));

  gateway_->on_order(&order);
}

void CtpTradeApi::OnRtnOrder(CThostFtdcOrderField *ctp_order) {
  // 听说CTP会收到别人的订单回报？判断一下
  if (ctp_order->InvestorID != investor_id_) {
    spdlog::warn(
        "[CtpTradeApi::OnRtnOrder] Failed. Received unknown investor's");
    return;
  }

  if (!ctp_order) {
    spdlog::error("[CtpTradeApi::OnRtnOrder] Failed. ctp_order is nullptr");
    return;
  }

  const auto *contract = ContractTable::get_by_symbol(ctp_order->InstrumentID);
  if (!contract) {
    spdlog::error("[CtpTradeApi::OnRspOrderInsert] Contract not found");
    return;
  }

  Order order;
  order.ticker_index = contract->index;
  order.order_id =
      get_order_id(order.ticker_index, std::stoi(ctp_order->OrderRef));
  order.direction = direction(ctp_order->Direction);
  order.offset = offset(ctp_order->CombOffsetFlag[0]);
  order.price = ctp_order->LimitPrice;
  order.volume_traded = ctp_order->VolumeTraded;
  order.volume = ctp_order->VolumeTotalOriginal;
  // order.insert_time = ctp_order->InsertTime;
  order.type = order_type(ctp_order->OrderPriceType);
  if (ctp_order->OrderSubmitStatus == THOST_FTDC_OSS_InsertRejected)
    order.status = OrderStatus::REJECTED;
  else if (ctp_order->OrderSubmitStatus == THOST_FTDC_OSS_CancelRejected)
    order.status = OrderStatus::CANCEL_REJECTED;
  else
    order.status = order_status(ctp_order->OrderStatus);

  spdlog::debug(
      "[CtpTradeApi::OnRtnOrder] Success. Order ID: {}, Instrument: {}, "
      "Exchange: {}, Direction: {}, Offset: {}, Origin Volume: {}, "
      "Traded: {}, Price: {:.2f}, Status: {}, Status Msg: {}",
      order.order_id, contract->symbol, contract->exchange,
      to_string(order.direction), to_string(order.offset), order.volume,
      order.volume_traded, order.price, to_string(order.status),
      gb2312_to_utf8(ctp_order->StatusMsg));

  gateway_->on_order(&order);
}

void CtpTradeApi::OnRtnTrade(CThostFtdcTradeField *trade) {
  if (trade->InvestorID != investor_id_) {
    spdlog::warn(
        "[CtpTradeApi::OnRtnTrade] Failed. Received unknown investor's");
    return;
  }

  const auto *contract = ContractTable::get_by_symbol(trade->InstrumentID);
  if (!contract) {
    spdlog::error("[CtpTradeApi::OnRtnTrade] Contract not found");
    return;
  }

  Trade td;
  td.ticker_index = contract->index;
  td.order_id = get_order_id(td.ticker_index, std::stoi(trade->OrderRef));
  td.trade_id = std::stoi(trade->TradeID);
  // td.trade_time = trade->TradeTime;
  td.direction = direction(trade->Direction);
  td.offset = offset(trade->OffsetFlag);
  td.price = trade->Price;
  td.volume = trade->Volume;

  spdlog::debug(
      "[CtpTradeApi::OnRtnTrade] Success. Order ID: {}, Instrument: {}, "
      "Exchange: {}, Trade ID: {}, Trade Time: {}, Direction: {}, "
      "Offset: {}, Price: {:.2f}, Volume: {}",
      td.order_id, contract->symbol, contract->exchange, td.trade_id,
      td.trade_time, to_string(td.direction), to_string(td.offset), td.price,
      td.volume);
  gateway_->on_trade(&td);
}

bool CtpTradeApi::cancel_order(uint64_t order_id) {
  if (!is_logon_) return false;

  CThostFtdcInputOrderActionField req;
  memset(&req, 0, sizeof(req));

  uint64_t ticker_index = (order_id >> 32) & 0xffffffff;
  int order_ref = order_id & 0xffffffff;
  const auto *contract = ContractTable::get_by_index(ticker_index);
  if (!contract) {
    spdlog::error("[CtpTradeApi::cancel_order] Contract not found");
    return false;
  }

  strncpy(req.InstrumentID, contract->symbol.c_str(), sizeof(req.InstrumentID));
  strncpy(req.ExchangeID, contract->exchange.c_str(), sizeof(req.ExchangeID));
  snprintf(req.OrderRef, sizeof(req.OrderRef), "%d", order_ref);
  strncpy(req.BrokerID, broker_id_.c_str(), sizeof(req.BrokerID));
  strncpy(req.InvestorID, investor_id_.c_str(), sizeof(req.InvestorID));
  req.ActionFlag = THOST_FTDC_AF_Delete;
  req.FrontID = front_id_;
  req.SessionID = session_id_;

  if (ctp_api_->ReqOrderAction(&req, next_req_id()) != 0) {
    spdlog::error(
        "[CtpTradeApi::cancel_order] Failed. Failed to ReqOrderAction");
    return false;
  }

  return true;
}

void CtpTradeApi::OnRspOrderAction(CThostFtdcInputOrderActionField *action,
                                   CThostFtdcRspInfoField *rsp_info, int req_id,
                                   bool is_last) {
  if (action->InvestorID != investor_id_) {
    spdlog::warn(
        "[CtpTradeApi::OnRspOrderAction] Failed. Received unknown investor's");
    return;
  }

  spdlog::error("[CtpTradeApi::OnRspOrderAction] Failed. Rejected. Reason: {}",
                gb2312_to_utf8(rsp_info->ErrorMsg));

  const auto *contract = ContractTable::get_by_symbol(action->InstrumentID);
  if (!contract) {
    spdlog::error("[CtpTradeApi::OnRspOrderAction] Contract not found");
    return;
  }

  Order order;
  order.ticker_index = contract->index;
  order.order_id =
      get_order_id(order.ticker_index, std::stoi(action->OrderRef));
  order.status = OrderStatus::CANCEL_REJECTED;

  gateway_->on_order(&order);
}

bool CtpTradeApi::query_contract(const std::string &ticker) {
  if (!is_logon_) return false;

  std::unique_lock<std::mutex> lock(query_mutex_);

  std::string symbol, exchange;
  ticker_split(ticker, &symbol, &exchange);

  CThostFtdcQryInstrumentField req;
  memset(&req, 0, sizeof(req));
  strncpy(req.InstrumentID, symbol.c_str(), sizeof(req.InstrumentID));
  strncpy(req.ExchangeID, exchange.c_str(), sizeof(req.ExchangeID));

  reset_sync();
  if (ctp_api_->ReqQryInstrument(&req, next_req_id()) != 0) {
    spdlog::error(
        "[CtpTradeApi::query_contract] Failed. Failed to ReqQryInstrument");
    return false;
  }

  return wait_sync();
}

void CtpTradeApi::OnRspQryInstrument(CThostFtdcInstrumentField *instrument,
                                     CThostFtdcRspInfoField *rsp_info,
                                     int req_id, bool is_last) {
  if (is_error_rsp(rsp_info)) {
    spdlog::error("[CtpTradeApi::OnRspQryInstrument] Failed. Error Msg: {}",
                  gb2312_to_utf8(rsp_info->ErrorMsg));
    error();
    return;
  }

  if (!instrument) {
    spdlog::error(
        "[CtpTradeApi::OnRspQryInstrument] Failed. instrument is nullptr");
    error();
    return;
  }

  spdlog::debug(
      "[CtpTradeApi::OnRspQryInstrument] Success. Instrument: {}, Exchange: {}",
      instrument->InstrumentID, instrument->ExchangeID);

  Contract contract;
  contract.product_type = product_type(instrument->ProductClass);
  contract.symbol = instrument->InstrumentID;
  contract.exchange = instrument->ExchangeID;
  contract.ticker = to_ticker(contract.symbol, contract.exchange);
  contract.name = gb2312_to_utf8(instrument->InstrumentName);
  contract.product_type = product_type(instrument->ProductClass);
  contract.size = instrument->VolumeMultiple;
  contract.price_tick = instrument->PriceTick;
  contract.max_market_order_volume = instrument->MaxMarketOrderVolume;
  contract.min_market_order_volume = instrument->MinMarketOrderVolume;
  contract.max_limit_order_volume = instrument->MaxLimitOrderVolume;
  contract.min_limit_order_volume = instrument->MinLimitOrderVolume;
  contract.delivery_year = instrument->DeliveryYear;
  contract.delivery_month = instrument->DeliveryMonth;

  gateway_->on_contract(&contract);

  if (is_last) done();
}

bool CtpTradeApi::query_position(const std::string &ticker) {
  if (!is_logon_) return false;

  std::unique_lock<std::mutex> lock(query_mutex_);

  std::string symbol, exchange;
  ticker_split(ticker, &symbol, &exchange);

  CThostFtdcQryInvestorPositionField req;
  memset(&req, 0, sizeof(req));
  strncpy(req.BrokerID, broker_id_.c_str(), sizeof(req.BrokerID));
  strncpy(req.InvestorID, investor_id_.c_str(), sizeof(req.InvestorID));
  strncpy(req.InstrumentID, symbol.c_str(), symbol.size());
  strncpy(req.ExchangeID, exchange.c_str(), sizeof(req.ExchangeID));

  reset_sync();
  if (ctp_api_->ReqQryInvestorPosition(&req, next_req_id()) != 0) {
    spdlog::error(
        "[CtpTradeApi::query_position] Failed. Failed to "
        "ReqQryInvestorPosition");
    return false;
  }

  return wait_sync();
}

void CtpTradeApi::OnRspQryInvestorPosition(
    CThostFtdcInvestorPositionField *position, CThostFtdcRspInfoField *rsp_info,
    int req_id, bool is_last) {
  if (is_error_rsp(rsp_info)) {
    spdlog::error(
        "[CtpTradeApi::OnRspQryInvestorPosition] Failed. Error Msg: {}",
        gb2312_to_utf8(rsp_info->ErrorMsg));
    pos_cache_.clear();
    error();
    return;
  }

  if (position) {
    const auto *contract = ContractTable::get_by_symbol(position->InstrumentID);
    if (!contract) {
      spdlog::error(
          "[CtpTradeApi::OnRspQryInvestorPosition] Contract not found");
      return;
    }

    auto ticker = to_ticker(position->InstrumentID, position->ExchangeID);
    auto &pos = pos_cache_[contract->index];
    pos.ticker_index = contract->index;

    bool is_long_pos = position->PosiDirection == THOST_FTDC_PD_Long;
    auto &pos_detail = is_long_pos ? pos.long_pos : pos.short_pos;
    if (contract->exchange == kSHFE || contract->exchange == kINE)
      pos_detail.yd_volume = position->YdPosition;
    else
      pos_detail.yd_volume = position->Position - position->TodayPosition;

    if (is_long_pos)
      pos_detail.frozen += position->LongFrozen;
    else
      pos_detail.frozen += position->ShortFrozen;

    pos_detail.volume = position->Position;
    pos_detail.float_pnl = position->PositionProfit;

    if (pos_detail.volume > 0 && contract->size > 0)
      pos_detail.cost_price =
          position->PositionCost / (pos_detail.volume * contract->size);

    spdlog::debug(
        "[CtpTradeApi::OnRspQryInvestorPosition] ticker: {}, long: {}, short: "
        "{}",
        contract->ticker, pos.long_pos.volume, pos.short_pos.volume);
  }

  if (is_last) {
    for (auto &[ticker_index, pos] : pos_cache_) gateway_->on_position(&pos);
    pos_cache_.clear();
    done();
  }
}

bool CtpTradeApi::query_account() {
  if (!is_logon_) return false;

  std::unique_lock<std::mutex> lock(query_mutex_);

  CThostFtdcQryTradingAccountField req;
  memset(&req, 0, sizeof(req));
  strncpy(req.BrokerID, broker_id_.c_str(), sizeof(req.BrokerID));
  strncpy(req.InvestorID, investor_id_.c_str(), sizeof(req.InvestorID));

  reset_sync();
  if (ctp_api_->ReqQryTradingAccount(&req, next_req_id()) != 0) {
    spdlog::error(
        "[CtpTradeApi::query_account] Failed. Failed to "
        "ReqQryTradingAccount");
    return false;
  }

  return wait_sync();
}

void CtpTradeApi::OnRspQryTradingAccount(
    CThostFtdcTradingAccountField *trading_account,
    CThostFtdcRspInfoField *rsp_info, int req_id, bool is_last) {
  if (!is_last) return;

  if (is_error_rsp(rsp_info)) {
    spdlog::error("[CtpTradeApi::OnRspQryTradingAccount] Failed. ErrorMsg: {}",
                  gb2312_to_utf8(rsp_info->ErrorMsg));
    error();
    return;
  }

  spdlog::debug(
      "[CtpTradeApi::OnRspQryTradingAccount] Success. "
      "Account ID: {}, Balance: {}, Frozen: {}",
      trading_account->AccountID, trading_account->Balance,
      trading_account->FrozenMargin);

  Account account;
  account.account_id = std::stoul(trading_account->AccountID);
  account.balance = trading_account->Balance;
  account.frozen = trading_account->FrozenCash + trading_account->FrozenMargin +
                   trading_account->FrozenCommission;

  gateway_->on_account(&account);
  done();
}

bool CtpTradeApi::query_orders() {
  if (!is_logon_) return false;

  std::unique_lock<std::mutex> lock(query_mutex_);

  CThostFtdcQryOrderField req;
  memset(&req, 0, sizeof(req));
  strncpy(req.BrokerID, broker_id_.c_str(), sizeof(req.BrokerID));
  strncpy(req.InvestorID, investor_id_.c_str(), sizeof(req.InvestorID));

  reset_sync();
  if (ctp_api_->ReqQryOrder(&req, next_req_id()) != 0) {
    spdlog::error("[CtpTradeApi::query_orders] Failed. Failed to ReqQryOrder");
    return false;
  }

  return wait_sync();
}

void CtpTradeApi::OnRspQryOrder(CThostFtdcOrderField *order,
                                CThostFtdcRspInfoField *rsp_info, int req_id,
                                bool is_last) {
  // TODO(kevin)

  if (is_error_rsp(rsp_info)) {
    spdlog::error("[CtpTradeApi::OnRspQryOrder] Failed. ErrorMsg: {}",
                  gb2312_to_utf8(rsp_info->ErrorMsg));
    error();
    return;
  }

  if (order && (order->OrderStatus == THOST_FTDC_OST_NoTradeQueueing ||
                order->OrderStatus == THOST_FTDC_OST_PartTradedQueueing)) {
    spdlog::info(
        "[CtpTradeApi::OnRspQryOrder] Cancel all orders on startup. Ticker: "
        "{}.{}, "
        "OrderSysID: {}, OriginalVolume: {}, Traded: {}, StatusMsg: {}",
        order->InstrumentID, order->ExchangeID, order->OrderSysID,
        order->VolumeTotalOriginal, order->VolumeTraded,
        gb2312_to_utf8(order->StatusMsg));

    CThostFtdcInputOrderActionField req;
    memset(&req, 0, sizeof(req));

    strncpy(req.InstrumentID, order->InstrumentID, sizeof(req.InstrumentID));
    strncpy(req.ExchangeID, order->ExchangeID, sizeof(req.ExchangeID));
    strncpy(req.OrderSysID, order->OrderSysID, sizeof(req.OrderSysID));
    strncpy(req.BrokerID, broker_id_.c_str(), sizeof(req.BrokerID));
    strncpy(req.InvestorID, investor_id_.c_str(), sizeof(req.InvestorID));
    req.ActionFlag = THOST_FTDC_AF_Delete;

    if (ctp_api_->ReqOrderAction(&req, next_req_id()) != 0)
      spdlog::error(
          "[CtpTradeApi::OnRspQryOrder] Failed to call ReqOrderAction");
  }

  if (is_last) done();
}

bool CtpTradeApi::query_trades() {
  if (!is_logon_) return false;

  std::unique_lock<std::mutex> lock(query_mutex_);

  CThostFtdcQryTradeField req;
  memset(&req, 0, sizeof(req));
  strncpy(req.BrokerID, broker_id_.c_str(), sizeof(req.BrokerID));
  strncpy(req.InvestorID, investor_id_.c_str(), sizeof(req.InvestorID));

  reset_sync();
  if (ctp_api_->ReqQryTrade(&req, next_req_id()) != 0) {
    spdlog::error("[CtpTradeApi::query_trades] Failed. Failed to ReqQryTrade");
    return false;
  }

  return wait_sync();
}

void CtpTradeApi::OnRspQryTrade(CThostFtdcTradeField *trade,
                                CThostFtdcRspInfoField *rsp_info, int req_id,
                                bool is_last) {
  // TODO(kevin)
  if (!is_last) return;

  if (is_error_rsp(rsp_info)) {
    spdlog::error("[CtpTradeApi::OnRspQryTrade] Failed. ErrorMsg: {}",
                  gb2312_to_utf8(rsp_info->ErrorMsg));
    error();
    return;
  }

  done();
}

bool CtpTradeApi::query_margin_rate(const std::string &ticker) {
  std::unique_lock<std::mutex> lock(query_mutex_);

  CThostFtdcQryInstrumentMarginRateField req;
  memset(&req, 0, sizeof(req));
  req.HedgeFlag = THOST_FTDC_HF_Speculation;

  std::string symbol, exchange;
  ticker_split(ticker, &symbol, &exchange);

  strncpy(req.BrokerID, broker_id_.c_str(), sizeof(req.BrokerID));
  strncpy(req.InvestorID, investor_id_.c_str(), sizeof(req.InvestorID));
  strncpy(req.InvestUnitID, investor_id_.c_str(), sizeof(req.InvestUnitID));
  strncpy(req.InstrumentID, symbol.c_str(), sizeof(req.InstrumentID));
  strncpy(req.ExchangeID, symbol.c_str(), sizeof(req.ExchangeID));

  reset_sync();
  if (ctp_api_->ReqQryInstrumentMarginRate(&req, next_req_id()) != 0) {
    spdlog::error(
        "[CtpTradeApi::query_margin_rate] Failed. "
        "Failed to call ReqQryInstrumentMarginRate");
    return false;
  }

  return wait_sync();
}

void CtpTradeApi::OnRspQryInstrumentMarginRate(
    CThostFtdcInstrumentMarginRateField *margin_rate,
    CThostFtdcRspInfoField *rsp_info, int req_id, bool is_last) {
  if (is_error_rsp(rsp_info)) {
    spdlog::error(
        "[CtpTradeApi::OnRspQryInstrumentMarginRate] Failed. ErrorMsg: {}",
        gb2312_to_utf8(rsp_info->ErrorMsg));
    error();
    return;
  }

  if (margin_rate) {
    // TODO(kevin)
  }

  if (is_last) done();
}

bool CtpTradeApi::query_commision_rate() {}

}  // namespace ft
