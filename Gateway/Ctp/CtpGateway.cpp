// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "Ctp/CtpGateway.h"

#include <ThostFtdcMdApi.h>
#include <ThostFtdcTraderApi.h>
#include <spdlog/spdlog.h>

#include "ContractTable.h"
#include "Core/Constants.h"

namespace ft {

CtpGateway::CtpGateway(TradingEngineInterface *engine) : Gateway(engine) {
  trade_spi_.reset(new CtpTradeSpi(this));
  md_spi_.reset(new CtpMdSpi(this));
}

CtpGateway::~CtpGateway() {
  error();
  logout();
}

bool CtpGateway::login(const LoginParams &params) {
  if (params.broker_id().size() > sizeof(TThostFtdcBrokerIDType) ||
      params.broker_id().empty() ||
      params.investor_id().size() > sizeof(TThostFtdcUserIDType) ||
      params.investor_id().empty() ||
      params.passwd().size() > sizeof(TThostFtdcPasswordType) ||
      params.passwd().empty() || params.front_addr().empty()) {
    spdlog::error("[CtpGateway::login] Failed. Invalid login params");
    return false;
  }

  if (!params.front_addr().empty()) {
    if (!login_into_trading_server(params)) {
      return false;
    }
  }

  if (!params.md_server_addr().empty()) {
    if (!login_into_md_server(params)) {
      return false;
    }
  }

  return true;
}

bool CtpGateway::login_into_trading_server(const LoginParams &params) {
  trade_api_.reset(CThostFtdcTraderApi::CreateFtdcTraderApi());
  if (!trade_api_) {
    spdlog::error("[CtpGateway::login] Failed. Failed to CreateFtdcTraderApi");
    return false;
  }

  front_addr_ = params.front_addr();
  broker_id_ = params.broker_id();
  investor_id_ = params.investor_id();

  trade_api_->SubscribePrivateTopic(THOST_TERT_QUICK);
  trade_api_->RegisterSpi(trade_spi_.get());
  trade_api_->RegisterFront(const_cast<char *>(params.front_addr().c_str()));
  trade_api_->Init();
  while (!is_connected_) {
    if (is_error_) {
      spdlog::error("[CtpGateway::login] Failed. Cannot connect to {}",
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
    if (trade_api_->ReqAuthenticate(&auth_req, next_req_id()) != 0) {
      spdlog::error("[CtpGateway::login] Failed. Failed to ReqAuthenticate");
      return false;
    }

    if (!wait_sync()) {
      spdlog::error("[CtpGateway::login] Failed. Failed to authenticate");
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
  if (trade_api_->ReqUserLogin(&login_req, next_req_id()) != 0) {
    spdlog::error("[CtpGateway::login] Failed. Failed to ReqUserLogin");
    return false;
  }

  if (!wait_sync()) {
    spdlog::error("[CtpGateway::login] Failed. Failed to login");
    return false;
  }

  CThostFtdcQrySettlementInfoField settlement_req;
  memset(&settlement_req, 0, sizeof(settlement_req));
  strncpy(settlement_req.BrokerID, broker_id_.c_str(),
          sizeof(settlement_req.BrokerID));
  strncpy(settlement_req.InvestorID, investor_id_.c_str(),
          sizeof(settlement_req.InvestorID));

  reset_sync();
  if (trade_api_->ReqQrySettlementInfo(&settlement_req, next_req_id()) != 0) {
    spdlog::error("[CtpGateway::login] Failed. Failed to ReqQrySettlementInfo");
    return false;
  }

  if (!wait_sync()) {
    spdlog::error("[CtpGateway::login] Failed. Failed to query settlement");
    return false;
  }

  CThostFtdcSettlementInfoConfirmField confirm_req;
  memset(&confirm_req, 0, sizeof(confirm_req));
  strncpy(confirm_req.BrokerID, broker_id_.c_str(),
          sizeof(confirm_req.BrokerID));
  strncpy(confirm_req.InvestorID, investor_id_.c_str(),
          sizeof(confirm_req.InvestorID));

  reset_sync();
  if (trade_api_->ReqSettlementInfoConfirm(&confirm_req, next_req_id()) != 0) {
    spdlog::error(
        "[CtpGateway::login] Failed. Failed to ReqSettlementInfoConfirm");
    return false;
  }

  if (!wait_sync()) {
    spdlog::error(
        "[CtpGateway::login] Failed. Failed to confirm settlement info");
    return false;
  }

  is_logon_ = true;

  if (!query_orders()) {
    spdlog::error("[CtpGateway::login] Failed. Failed to query_orders");
    return false;
  }

  std::this_thread::sleep_for(std::chrono::seconds(1));
  return true;
}

/* 登录到行情服务器
 */
bool CtpGateway::login_into_md_server(const LoginParams &params) {
  md_api_.reset(CThostFtdcMdApi::CreateFtdcMdApi());
  if (!md_api_) {
    spdlog::error("[CtpGateway::login] Failed to create CTP MD API");
    return false;
  }

  md_api_->RegisterSpi(md_spi_.get());
  md_api_->RegisterFront(const_cast<char *>(params.md_server_addr().c_str()));
  md_api_->Init();

  for (;;) {
    if (is_md_error_) return false;

    if (is_md_connected_) break;
  }

  CThostFtdcReqUserLoginField login_req;
  memset(&login_req, 0, sizeof(login_req));
  strncpy(login_req.BrokerID, params.broker_id().c_str(),
          sizeof(login_req.BrokerID));
  strncpy(login_req.UserID, params.investor_id().c_str(),
          sizeof(login_req.UserID));
  strncpy(login_req.Password, params.passwd().c_str(),
          sizeof(login_req.Password));
  if (md_api_->ReqUserLogin(&login_req, next_req_id()) != 0) {
    spdlog::error("[CtpMdApi::login] Invalid user-login field");
    return false;
  }

  for (;;) {
    if (is_md_error_) return false;

    if (is_md_login_) break;
  }

  std::vector<char *> sub_list;
  std::string symbol;
  std::string exchange;
  for (const auto &ticker : params.subscribed_list()) {
    ticker_split(ticker, &symbol, &exchange);
    subscribed_list_.emplace_back(std::move(symbol));
  }

  for (const auto &p : subscribed_list_)
    sub_list.emplace_back(const_cast<char *>(p.c_str()));

  if (md_api_->SubscribeMarketData(sub_list.data(), sub_list.size()) != 0) {
    spdlog::error("[CtpMdApi::login] Failed to subscribe");
    return false;
  }

  return true;
}

void CtpGateway::logout() {
  CThostFtdcUserLogoutField req;
  strncpy(req.BrokerID, broker_id_.c_str(), sizeof(req.BrokerID));
  strncpy(req.UserID, investor_id_.c_str(), sizeof(req.UserID));

  if (is_logon_) {
    if (trade_api_->ReqUserLogout(&req, next_req_id()) != 0) return;

    while (is_logon_) continue;
  }

  if (is_md_login_) {
    CThostFtdcUserLogoutField req;
    strncpy(req.BrokerID, broker_id_.c_str(), sizeof(req.BrokerID));
    strncpy(req.UserID, investor_id_.c_str(), sizeof(req.UserID));
    if (md_api_->ReqUserLogout(&req, next_req_id()) != 0) return;

    while (is_md_login_) continue;
  }
}

void CtpGateway::OnFrontConnected() {
  spdlog::debug("[CtpGateway::OnFrontConnected] Success. Connected to {}",
                front_addr_);
  is_error_ = false;
  is_connected_ = true;
}

void CtpGateway::OnFrontDisconnected(int reason) {
  spdlog::error("[CtpGateway::OnFrontDisconnected] . Disconnected from {}",
                front_addr_);
  error();
  is_connected_ = false;
}

void CtpGateway::OnHeartBeatWarning(int time_lapse) {
  spdlog::warn(
      "[CtpGateway::OnHeartBeatWarning] Warn. No packet received for a period "
      "of time");
}

void CtpGateway::OnRspAuthenticate(
    CThostFtdcRspAuthenticateField *rsp_authenticate_field,
    CThostFtdcRspInfoField *rsp_info, int req_id, bool is_last) {
  if (!is_last) return;

  if (is_error_rsp(rsp_info)) {
    spdlog::error("[CtpGateway::OnRspAuthenticate] Failed. ErrorMsg: {}",
                  gb2312_to_utf8(rsp_info->ErrorMsg));
    error();
    return;
  }

  spdlog::debug("[CTP::OnRspAuthenticate] Success. Investor ID: {}",
                investor_id_);
  done();
}

void CtpGateway::OnRspUserLogin(CThostFtdcRspUserLoginField *rsp_user_login,
                                CThostFtdcRspInfoField *rsp_info, int req_id,
                                bool is_last) {
  if (!is_last) return;

  if (is_error_rsp(rsp_info)) {
    spdlog::error("[CtpGateway::OnRspUserLogin] Failed. ErrorMsg: {}",
                  gb2312_to_utf8(rsp_info->ErrorMsg));
    error();
    return;
  }

  front_id_ = rsp_user_login->FrontID;
  session_id_ = rsp_user_login->SessionID;
  int max_order_ref = std::stoi(rsp_user_login->MaxOrderRef);
  next_order_ref_ = max_order_ref + 1;

  spdlog::debug(
      "[CtpGateway::OnRspUserLogin] Success. Login as {}. "
      "Front ID: {}, Session ID: {}, Max OrderRef: {}",
      investor_id_, front_id_, session_id_, max_order_ref);
  done();
}

void CtpGateway::OnRspQrySettlementInfo(
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

void CtpGateway::OnRspSettlementInfoConfirm(
    CThostFtdcSettlementInfoConfirmField *settlement_info_confirm,
    CThostFtdcRspInfoField *rsp_info, int req_id, bool is_last) {
  if (!is_last) return;

  if (is_error_rsp(rsp_info)) {
    spdlog::debug(
        "[CtpGateway::OnRspSettlementInfoConfirm] Failed. ErrorMsg: {}",
        gb2312_to_utf8(rsp_info->ErrorMsg));
    error();
    return;
  }

  spdlog::debug(
      "[CtpGateway::OnRspSettlementInfoConfirm] Success. Settlement "
      "confirmed");
  done();
}

void CtpGateway::OnRspUserLogout(CThostFtdcUserLogoutField *user_logout,
                                 CThostFtdcRspInfoField *rsp_info, int req_id,
                                 bool is_last) {
  spdlog::debug(
      "[CtpGateway::OnRspUserLogout] Success. Broker ID: {}, Investor ID: {}",
      user_logout->BrokerID, user_logout->UserID);
  is_logon_ = false;
}

uint64_t CtpGateway::send_order(const OrderReq *order) {
  if (!is_logon_) {
    spdlog::error("[CtpGateway::send_order] Failed. Not logon");
    return 0;
  }

  const auto *contract = ContractTable::get_by_index(order->ticker_index);
  if (!contract) {
    spdlog::error("[CtpGateway::send_order] Contract not found");
    return 0;
  }

  uint64_t order_ref = next_order_ref();
  CThostFtdcInputOrderField req;
  memset(&req, 0, sizeof(req));
  strncpy(req.BrokerID, broker_id_.c_str(), sizeof(req.BrokerID));
  strncpy(req.InvestorID, investor_id_.c_str(), sizeof(req.InvestorID));
  strncpy(req.InstrumentID, contract->symbol.c_str(), sizeof(req.InstrumentID));
  strncpy(req.ExchangeID, contract->exchange.c_str(), sizeof(req.ExchangeID));
  snprintf(req.OrderRef, sizeof(req.OrderRef), "%d", order_ref);
  req.OrderPriceType = order_type(order->type);
  req.Direction = direction(order->direction);
  req.CombOffsetFlag[0] = offset(order->offset);
  req.LimitPrice = order->price;
  req.VolumeTotalOriginal = order->volume;
  req.CombHedgeFlag[0] = THOST_FTDC_HF_Speculation;
  req.ContingentCondition = THOST_FTDC_CC_Immediately;
  req.ForceCloseReason = THOST_FTDC_FCC_NotForceClose;
  req.MinVolume = 1;
  req.IsAutoSuspend = 0;
  req.UserForceClose = 0;

  if (order->type == OrderType::FAK) {
    req.TimeCondition = THOST_FTDC_TC_IOC;
    req.VolumeCondition = THOST_FTDC_VC_AV;
  } else if (order->type == OrderType::FOK) {
    req.TimeCondition = THOST_FTDC_TC_IOC;
    req.VolumeCondition = THOST_FTDC_VC_CV;
  } else {
    req.TimeCondition = THOST_FTDC_TC_GFD;
    req.VolumeCondition = THOST_FTDC_VC_AV;
  }

  std::unique_lock<std::mutex> lock(order_mutex_);
  if (trade_api_->ReqOrderInsert(&req, next_req_id()) != 0) return 0;

  OrderDetail detail;
  detail.contract = contract;
  order_details_.emplace(order_ref, detail);

  return order_ref;
}

void CtpGateway::OnRspOrderInsert(CThostFtdcInputOrderField *order,
                                  CThostFtdcRspInfoField *rsp_info, int req_id,
                                  bool is_last) {
  if (!order) {
    spdlog::warn("[CtpGateway::OnRspOrderInsert] nullptr");
    return;
  }

  if (order->InvestorID != investor_id_) {
    spdlog::warn(
        "[CtpGateway::OnRspOrderInsert] Failed. "
        "= =#Receive RspOrderInsert of other investor");
    return;
  }

  int order_ref;
  try {
    order_ref = std::stoi(order->OrderRef);
  } catch (...) {
    spdlog::error("[CtpGateway::OnRspOrderInsert] Invalid order ref");
    return;
  }

  spdlog::error(
      "[CtpGateway::OnRspOrderInsert] Rejected. OrderRef: {}, Status: "
      "Rejected, ErrorMsg: {}",
      order->OrderRef, gb2312_to_utf8(rsp_info->ErrorMsg));

  std::unique_lock<std::mutex> lock(order_mutex_);
  auto iter = order_details_.find(order_ref);
  if (order_details_.find(order_ref) == order_details_.end()) {
    spdlog::error(
        "[CtpGateway::OnRspOrderInsert] Order not found. OrderRef: {}",
        order_ref);
    return;
  }
  order_details_.erase(iter);
  lock.unlock();

  engine_->on_order_rejected(order_ref);
}

void CtpGateway::OnRtnOrder(CThostFtdcOrderField *order) {
  if (!order) {
    spdlog::warn("[CtpGateway::OnRtnOrder] nullptr");
    return;
  }

  // 听说CTP会收到别人的订单回报？判断一下
  if (order->InvestorID != investor_id_) {
    spdlog::warn("[CtpGateway::OnRtnOrder] Failed. Unknown order");
    return;
  }

  int order_ref;
  try {
    order_ref = std::stoi(order->OrderRef);
  } catch (...) {
    spdlog::error("[CtpGateway::OnRspOrderInsert] Invalid order ref");
    return;
  }

  std::unique_lock<std::mutex> lock(order_mutex_);
  auto iter = order_details_.find(order_ref);
  if (order_details_.find(order_ref) == order_details_.end()) {
    spdlog::error("[CtpGateway::OnRtnOrder] Order not found. OrderRef: {}",
                  order_ref);
    return;
  }

  // 被拒单或撤销被拒，回调相应函数
  if (order->OrderSubmitStatus == THOST_FTDC_OSS_InsertRejected) {
    engine_->on_order_rejected(order_ref);
    return;
  } else if (order->OrderSubmitStatus == THOST_FTDC_OSS_CancelRejected) {
    engine_->on_order_cancel_rejected(order_ref);
    return;
  }

  // 如果只是被CTP接收，则直接返回，只能撤被交易所接受的单
  if (order->OrderStatus == THOST_FTDC_OST_Unknown ||
      order->OrderStatus == THOST_FTDC_OST_NoTradeNotQueueing)
    return;

  auto detail = &iter->second;

  // 被交易所接收，则回调on_order_accepted
  if (!detail->accepted_ack) {
    engine_->on_order_accepted(order_ref);
    detail->accepted_ack = true;
  }

  // 处理撤单
  if (order->OrderStatus == THOST_FTDC_OST_PartTradedNotQueueing ||
      order->OrderStatus == THOST_FTDC_OST_Canceled) {
    // 撤单都是一次性撤销所有未成交订单
    // 这里是为了防止重接收到撤单回执
    if (detail->canceled_vol == 0) {
      detail->canceled_vol = order->VolumeTotalOriginal - order->VolumeTraded;
      engine_->on_order_canceled(order_ref, detail->canceled_vol);
    }

    // 这里是处理撤单比回调先到的情况，如果撤单比成交回执先到，则继续等待成交回执到来
    if (detail->canceled_vol + detail->traded_vol == detail->original_vol)
      order_details_.erase(iter);
  }
}

void CtpGateway::OnRtnTrade(CThostFtdcTradeField *trade) {
  if (!trade) {
    spdlog::warn("[CtpGateway::OnRtnTrade] nullptr");
    return;
  }

  if (trade->InvestorID != investor_id_) {
    spdlog::warn("[CtpGateway::OnRtnTrade] Failed. Recv unknown trade");
    return;
  }

  int order_ref;
  try {
    order_ref = std::stoi(trade->OrderRef);
  } catch (...) {
    spdlog::error("[CtpGateway::OnRtnTrade] Invalid OrderRef");
    return;
  }

  std::unique_lock<std::mutex> lock(order_mutex_);
  auto iter = order_details_.find(order_ref);
  if (iter == order_details_.end()) {
    spdlog::error("[CtpGateway::OnRtnTrade] Order not found. OrderRef: {}",
                  order_ref);
    return;
  }

  auto detail = &iter->second;
  detail->traded_vol += trade->Volume;
  if (detail->traded_vol + detail->canceled_vol == detail->original_vol)
    order_details_.erase(iter);
  lock.unlock();

  engine_->on_order_traded(order_ref, trade->Volume, trade->Price);
}

bool CtpGateway::cancel_order(uint64_t order_id) {
  if (!is_logon_) return false;

  CThostFtdcInputOrderActionField req;
  memset(&req, 0, sizeof(req));

  uint64_t ticker_index = (order_id >> 32) & 0xffffffff;
  int order_ref = order_id & 0xffffffff;
  const auto *contract = ContractTable::get_by_index(ticker_index);
  if (!contract) {
    spdlog::error("[CtpGateway::cancel_order] Contract not found");
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

  if (trade_api_->ReqOrderAction(&req, next_req_id()) != 0) {
    spdlog::error(
        "[CtpGateway::cancel_order] Failed. Failed to ReqOrderAction");
    return false;
  }

  return true;
}

void CtpGateway::OnRspOrderAction(CThostFtdcInputOrderActionField *action,
                                  CThostFtdcRspInfoField *rsp_info, int req_id,
                                  bool is_last) {
  if (!action) {
    spdlog::warn("[CtpGateway::OnRspOrderAction] nullptr");
  }

  if (action->InvestorID != investor_id_) {
    spdlog::warn("[CtpGateway::OnRspOrderAction] Failed. Recv unknown action");
    return;
  }

  spdlog::error("[CtpGateway::OnRspOrderAction] Failed. Rejected. Reason: {}",
                gb2312_to_utf8(rsp_info->ErrorMsg));

  int order_ref;
  try {
    order_ref = std::stoi(action->OrderRef);
  } catch (...) {
    spdlog::error("[CtpGateway::OnRspOrderAction] Invalid OrderRef");
    return;
  }

  std::unique_lock<std::mutex> lock(order_mutex_);
  if (order_details_.find(order_ref) == order_details_.end()) {
    spdlog::error(
        "[CtpGateway::OnRspOrderAction] Order not found. OrderRef: {}",
        order_ref);
    return;
  }
  lock.unlock();

  engine_->on_order_cancel_rejected(order_ref);
}

bool CtpGateway::query_contract(const std::string &ticker) {
  if (!is_logon_) return false;

  std::unique_lock<std::mutex> lock(query_mutex_);

  std::string symbol, exchange;
  ticker_split(ticker, &symbol, &exchange);

  CThostFtdcQryInstrumentField req;
  memset(&req, 0, sizeof(req));
  strncpy(req.InstrumentID, symbol.c_str(), sizeof(req.InstrumentID));
  strncpy(req.ExchangeID, exchange.c_str(), sizeof(req.ExchangeID));

  reset_sync();
  if (trade_api_->ReqQryInstrument(&req, next_req_id()) != 0) {
    spdlog::error(
        "[CtpGateway::query_contract] Failed. Failed to ReqQryInstrument");
    return false;
  }

  return wait_sync();
}

bool CtpGateway::query_contracts() { return query_contract(""); }

void CtpGateway::OnRspQryInstrument(CThostFtdcInstrumentField *instrument,
                                    CThostFtdcRspInfoField *rsp_info,
                                    int req_id, bool is_last) {
  if (is_error_rsp(rsp_info)) {
    spdlog::error("[CtpGateway::OnRspQryInstrument] Failed. Error Msg: {}",
                  gb2312_to_utf8(rsp_info->ErrorMsg));
    error();
    return;
  }

  if (!instrument) {
    spdlog::error(
        "[CtpGateway::OnRspQryInstrument] Failed. instrument is nullptr");
    error();
    return;
  }

  spdlog::debug(
      "[CtpGateway::OnRspQryInstrument] Success. Instrument: {}, Exchange: {}",
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

  engine_->on_query_contract(&contract);

  if (is_last) done();
}

bool CtpGateway::query_position(const std::string &ticker) {
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
  if (trade_api_->ReqQryInvestorPosition(&req, next_req_id()) != 0) {
    spdlog::error(
        "[CtpGateway::query_position] Failed. Failed to "
        "ReqQryInvestorPosition");
    return false;
  }

  return wait_sync();
}

bool CtpGateway::query_positions() { return query_position(""); }

void CtpGateway::OnRspQryInvestorPosition(
    CThostFtdcInvestorPositionField *position, CThostFtdcRspInfoField *rsp_info,
    int req_id, bool is_last) {
  if (is_error_rsp(rsp_info)) {
    spdlog::error(
        "[CtpGateway::OnRspQryInvestorPosition] Failed. Error Msg: {}",
        gb2312_to_utf8(rsp_info->ErrorMsg));
    pos_cache_.clear();
    error();
    return;
  }

  if (position) {
    const auto *contract = ContractTable::get_by_symbol(position->InstrumentID);
    if (!contract) {
      spdlog::error(
          "[CtpGateway::OnRspQryInvestorPosition] Contract not found");
      return;
    }

    auto ticker = to_ticker(position->InstrumentID, position->ExchangeID);
    auto &pos = pos_cache_[contract->index];
    pos.ticker_index = contract->index;

    bool is_long_pos = position->PosiDirection == THOST_FTDC_PD_Long;
    auto &pos_detail = is_long_pos ? pos.long_pos : pos.short_pos;
    if (contract->exchange == EX_SHFE || contract->exchange == EX_INE)
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
        "[CtpGateway::OnRspQryInvestorPosition] ticker: {}, long: {}, short: "
        "{}",
        contract->ticker, pos.long_pos.volume, pos.short_pos.volume);
  }

  if (is_last) {
    for (auto &[ticker_index, pos] : pos_cache_)
      engine_->on_query_position(&pos);
    pos_cache_.clear();
    done();
  }
}

bool CtpGateway::query_account() {
  if (!is_logon_) return false;

  std::unique_lock<std::mutex> lock(query_mutex_);

  CThostFtdcQryTradingAccountField req;
  memset(&req, 0, sizeof(req));
  strncpy(req.BrokerID, broker_id_.c_str(), sizeof(req.BrokerID));
  strncpy(req.InvestorID, investor_id_.c_str(), sizeof(req.InvestorID));

  reset_sync();
  if (trade_api_->ReqQryTradingAccount(&req, next_req_id()) != 0) {
    spdlog::error(
        "[CtpGateway::query_account] Failed. Failed to "
        "ReqQryTradingAccount");
    return false;
  }

  return wait_sync();
}

void CtpGateway::OnRspQryTradingAccount(
    CThostFtdcTradingAccountField *trading_account,
    CThostFtdcRspInfoField *rsp_info, int req_id, bool is_last) {
  if (!is_last) return;

  if (is_error_rsp(rsp_info)) {
    spdlog::error("[CtpGateway::OnRspQryTradingAccount] Failed. ErrorMsg: {}",
                  gb2312_to_utf8(rsp_info->ErrorMsg));
    error();
    return;
  }

  spdlog::debug(
      "[CtpGateway::OnRspQryTradingAccount] Success. "
      "Account ID: {}, Balance: {}, Frozen: {}",
      trading_account->AccountID, trading_account->Balance,
      trading_account->FrozenMargin);

  Account account;
  account.account_id = std::stoul(trading_account->AccountID);
  account.balance = trading_account->Balance;
  account.frozen = trading_account->FrozenCash + trading_account->FrozenMargin +
                   trading_account->FrozenCommission;

  engine_->on_query_account(&account);
  done();
}

bool CtpGateway::query_orders() {
  if (!is_logon_) return false;

  std::unique_lock<std::mutex> lock(query_mutex_);

  CThostFtdcQryOrderField req;
  memset(&req, 0, sizeof(req));
  strncpy(req.BrokerID, broker_id_.c_str(), sizeof(req.BrokerID));
  strncpy(req.InvestorID, investor_id_.c_str(), sizeof(req.InvestorID));

  reset_sync();
  if (trade_api_->ReqQryOrder(&req, next_req_id()) != 0) {
    spdlog::error("[CtpGateway::query_orders] Failed. Failed to ReqQryOrder");
    return false;
  }

  return wait_sync();
}

void CtpGateway::OnRspQryOrder(CThostFtdcOrderField *order,
                               CThostFtdcRspInfoField *rsp_info, int req_id,
                               bool is_last) {
  // TODO(kevin)

  if (is_error_rsp(rsp_info)) {
    spdlog::error("[CtpGateway::OnRspQryOrder] Failed. ErrorMsg: {}",
                  gb2312_to_utf8(rsp_info->ErrorMsg));
    error();
    return;
  }

  if (order && (order->OrderStatus == THOST_FTDC_OST_NoTradeQueueing ||
                order->OrderStatus == THOST_FTDC_OST_PartTradedQueueing)) {
    spdlog::info(
        "[CtpGateway::OnRspQryOrder] Cancel all orders on startup. Ticker: "
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

    if (trade_api_->ReqOrderAction(&req, next_req_id()) != 0)
      spdlog::error(
          "[CtpGateway::OnRspQryOrder] Failed to call ReqOrderAction");
  }

  if (is_last) done();
}

bool CtpGateway::query_trades() {
  if (!is_logon_) return false;

  std::unique_lock<std::mutex> lock(query_mutex_);

  CThostFtdcQryTradeField req;
  memset(&req, 0, sizeof(req));
  strncpy(req.BrokerID, broker_id_.c_str(), sizeof(req.BrokerID));
  strncpy(req.InvestorID, investor_id_.c_str(), sizeof(req.InvestorID));

  reset_sync();
  if (trade_api_->ReqQryTrade(&req, next_req_id()) != 0) {
    spdlog::error("[CtpGateway::query_trades] Failed. Failed to ReqQryTrade");
    return false;
  }

  return wait_sync();
}

void CtpGateway::OnRspQryTrade(CThostFtdcTradeField *trade,
                               CThostFtdcRspInfoField *rsp_info, int req_id,
                               bool is_last) {
  // TODO(kevin)
  if (!is_last) return;

  if (is_error_rsp(rsp_info)) {
    spdlog::error("[CtpGateway::OnRspQryTrade] Failed. ErrorMsg: {}",
                  gb2312_to_utf8(rsp_info->ErrorMsg));
    error();
    return;
  }

  done();
}

bool CtpGateway::query_margin_rate(const std::string &ticker) {
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
  if (trade_api_->ReqQryInstrumentMarginRate(&req, next_req_id()) != 0) {
    spdlog::error(
        "[CtpGateway::query_margin_rate] Failed. "
        "Failed to call ReqQryInstrumentMarginRate");
    return false;
  }

  return wait_sync();
}

void CtpGateway::OnRspQryInstrumentMarginRate(
    CThostFtdcInstrumentMarginRateField *margin_rate,
    CThostFtdcRspInfoField *rsp_info, int req_id, bool is_last) {
  if (is_error_rsp(rsp_info)) {
    spdlog::error(
        "[CtpGateway::OnRspQryInstrumentMarginRate] Failed. ErrorMsg: {}",
        gb2312_to_utf8(rsp_info->ErrorMsg));
    error();
    return;
  }

  if (margin_rate) {
    // TODO(kevin)
  }

  if (is_last) done();
}

void CtpGateway::OnFrontConnectedMD() {
  is_md_connected_ = true;
  spdlog::debug("[CtpGateway::OnFrontConnectedMD] Connected");
}

void CtpGateway::OnFrontDisconnectedMD(int reason) {
  is_md_error_ = true;
  is_md_connected_ = false;
  spdlog::error("[CtpGateway::OnFrontDisconnectedMD] Disconnected");
}

void CtpGateway::OnHeartBeatWarningMD(int time_lapse) {
  spdlog::warn(
      "[CtpGateway::OnHeartBeatWarningMD] Warn. No packet received for a "
      "period of time");
}

void CtpGateway::OnRspUserLoginMD(CThostFtdcRspUserLoginField *login_rsp,
                                  CThostFtdcRspInfoField *rsp_info, int req_id,
                                  bool is_last) {
  if (!is_last) return;

  if (is_error_rsp(rsp_info)) {
    spdlog::error("[CtpGateway::OnRspUserLogin] Failed. ErrorMsg: {}",
                  gb2312_to_utf8(rsp_info->ErrorMsg));
    is_md_error_ = true;
    return;
  }

  spdlog::debug("[CtpGateway::OnRspUserLogin] Success. Login as {}",
                investor_id_);
  is_md_login_ = true;
}

void CtpGateway::OnRspUserLogoutMD(CThostFtdcUserLogoutField *logout_rsp,
                                   CThostFtdcRspInfoField *rsp_info, int req_id,
                                   bool is_last) {
  spdlog::debug(
      "[CtpGateway::OnRspUserLogout] Success. Broker ID: {}, Investor ID: {}",
      logout_rsp->BrokerID, logout_rsp->UserID);
  is_md_login_ = false;
}

void CtpGateway::OnRspErrorMD(CThostFtdcRspInfoField *rsp_info, int req_id,
                              bool is_last) {
  spdlog::debug("[CtpGateway::OnRspError] ErrorMsg: {}",
                gb2312_to_utf8(rsp_info->ErrorMsg));
  is_md_login_ = false;
}

void CtpGateway::OnRspSubMarketData(
    CThostFtdcSpecificInstrumentField *instrument,
    CThostFtdcRspInfoField *rsp_info, int req_id, bool is_last) {
  if (is_error_rsp(rsp_info) || !instrument) {
    spdlog::error("[CtpGateway::OnRspSubMarketData] Failed. Error Msg: {}",
                  gb2312_to_utf8(rsp_info->ErrorMsg));
    return;
  }

  auto *contract = ContractTable::get_by_symbol(instrument->InstrumentID);
  if (!contract) {
    spdlog::error(
        "[CtpGateway::OnRspSubMarketData] Failed. ExchangeID not found in "
        "contract list. "
        "Maybe you should update the contract list. Symbol: {}",
        instrument->InstrumentID);
    return;
  }
  symbol2contract_.emplace(contract->symbol, contract);

  spdlog::debug("[CtpGateway::OnRspSubMarketData] Success. Ticker: {}",
                contract->ticker);
}

void CtpGateway::OnRspUnSubMarketData(
    CThostFtdcSpecificInstrumentField *instrument,
    CThostFtdcRspInfoField *rsp_info, int req_id, bool is_last) {}

void CtpGateway::OnRspSubForQuoteRsp(
    CThostFtdcSpecificInstrumentField *instrument,
    CThostFtdcRspInfoField *rsp_info, int req_id, bool is_last) {}

void CtpGateway::OnRspUnSubForQuoteRsp(
    CThostFtdcSpecificInstrumentField *instrument,
    CThostFtdcRspInfoField *rsp_info, int req_id, bool is_last) {}

void CtpGateway::OnRtnDepthMarketData(CThostFtdcDepthMarketDataField *md) {
  if (!md) {
    spdlog::error("[CtpGateway::OnRtnDepthMarketData] Failed. md is nullptr");
    return;
  }

  auto iter = symbol2contract_.find(md->InstrumentID);
  if (iter == symbol2contract_.end()) {
    spdlog::warn(
        "[CtpGateway::OnRtnDepthMarketData] Failed. ExchangeID not found in "
        "contract list. "
        "Maybe you should update the contract list. Symbol: {}",
        md->InstrumentID);
    return;
  }

  TickData tick;
  tick.ticker_index = iter->second->index;

  struct tm _tm;
  strptime(md->UpdateTime, "%H:%M:%S", &_tm);
  tick.time_sec = _tm.tm_sec + _tm.tm_min * 60 + _tm.tm_hour * 3600;
  tick.time_ms = md->UpdateMillisec;
  // tick.date = md->ActionDay;

  tick.volume = md->Volume;
  tick.turnover = md->Turnover;
  tick.open_interest = md->OpenInterest;
  tick.last_price = adjust_price(md->LastPrice);
  tick.open_price = adjust_price(md->OpenPrice);
  tick.highest_price = adjust_price(md->HighestPrice);
  tick.lowest_price = adjust_price(md->LowestPrice);
  tick.pre_close_price = adjust_price(md->PreClosePrice);
  tick.upper_limit_price = adjust_price(md->UpperLimitPrice);
  tick.lower_limit_price = adjust_price(md->LowerLimitPrice);

  tick.level = 5;
  tick.ask[0] = adjust_price(md->AskPrice1);
  tick.ask[1] = adjust_price(md->AskPrice2);
  tick.ask[2] = adjust_price(md->AskPrice3);
  tick.ask[3] = adjust_price(md->AskPrice4);
  tick.ask[4] = adjust_price(md->AskPrice5);
  tick.bid[0] = adjust_price(md->BidPrice1);
  tick.bid[1] = adjust_price(md->BidPrice2);
  tick.bid[2] = adjust_price(md->BidPrice3);
  tick.bid[3] = adjust_price(md->BidPrice4);
  tick.bid[4] = adjust_price(md->BidPrice5);
  tick.ask_volume[0] = md->AskVolume1;
  tick.ask_volume[1] = md->AskVolume2;
  tick.ask_volume[2] = md->AskVolume3;
  tick.ask_volume[3] = md->AskVolume4;
  tick.ask_volume[4] = md->AskVolume5;
  tick.bid_volume[0] = md->BidVolume1;
  tick.bid_volume[1] = md->BidVolume2;
  tick.bid_volume[2] = md->BidVolume3;
  tick.bid_volume[3] = md->BidVolume4;
  tick.bid_volume[4] = md->BidVolume5;

  spdlog::debug(
      "[CtpGateway::OnRtnDepthMarketData] Ticker: {}, Time MS: {}, "
      "LastPrice: {:.2f}, Volume: {}, Turnover: {}, Open Interest: {}",
      iter->second->ticker, tick.time_ms, tick.last_price, tick.volume,
      tick.turnover, tick.open_interest);

  engine_->on_tick(&tick);
}

void CtpGateway::OnRtnForQuoteRsp(CThostFtdcForQuoteRspField *for_quote_rsp) {}

}  // namespace ft
