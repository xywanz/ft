// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "ctp/CtpTradeApi.h"

#include <cassert>
#include <cstring>

#include <cppex/split.h>
#include <spdlog/spdlog.h>
#include <ThostFtdcUserApiDataType.h>
#include <ThostFtdcUserApiStruct.h>
#include <ThostFtdcTraderApi.h>

#include "Account.h"
#include "ctp/CtpCommon.h"
#include "ctp/FieldMapper.h"
#include "Contract.h"
#include "Position.h"
#include "Trade.h"

namespace ft {

CtpTradeApi::CtpTradeApi(GeneralApi* general_api)
  : general_api_(general_api) {
}

CtpTradeApi::~CtpTradeApi() {
  if (ctp_api_)
    ctp_api_->Release();
}

bool CtpTradeApi::login(const LoginParams& params) {
  if (params.broker_id().size() > sizeof(TThostFtdcBrokerIDType) ||
      params.broker_id().empty() ||
      params.investor_id().size() > sizeof(TThostFtdcUserIDType) ||
      params.investor_id().empty() ||
      params.passwd().size() > sizeof(TThostFtdcPasswordType) ||
      params.passwd().empty() ||
      params.front_addr().empty()) {
    spdlog::error("[CTP] login. Invalid login params");
    return false;
  }

  ctp_api_ = CThostFtdcTraderApi::CreateFtdcTraderApi();
  if (!ctp_api_) {
    spdlog::error("[CTP] login. Failed to create CTP API");
    return false;
  }

  front_addr_ = params.front_addr();
  broker_id_ = params.broker_id();
  investor_id_ = params.investor_id();

  ctp_api_->SubscribePrivateTopic(THOST_TERT_QUICK);
  ctp_api_->RegisterSpi(this);
  ctp_api_->RegisterFront(const_cast<char*>(params.front_addr().c_str()));
  ctp_api_->Init();
  while (!is_connected_) {
    if (is_error_)
      return false;
  }

  if (!params.auth_code().empty()) {
    CThostFtdcReqAuthenticateField auth_req;
    memset(&auth_req, 0, sizeof(auth_req));
    strncpy(auth_req.BrokerID, params.broker_id().c_str(), sizeof(auth_req.BrokerID));
    strncpy(auth_req.UserID, params.investor_id().c_str(), sizeof(auth_req.UserID));
    strncpy(auth_req.AuthCode, params.auth_code().c_str(), sizeof(auth_req.AuthCode));
    strncpy(auth_req.AppID, params.app_id().c_str(), sizeof(auth_req.AppID));
    if (ctp_api_->ReqAuthenticate(&auth_req, next_req_id()) != 0) {
      spdlog::error("[CTP] login. Invalid user-login field");
      return false;
    }

    while (is_authenticated_) {
      if (is_error_) {
        spdlog::error("[CTP] login. Failed to authenticate");
        return false;
      }
    }
  }

  CThostFtdcReqUserLoginField login_req;
  memset(&login_req, 0, sizeof(login_req));
  strncpy(login_req.BrokerID, params.broker_id().c_str(), sizeof(login_req.BrokerID));
  strncpy(login_req.UserID, params.investor_id().c_str(), sizeof(login_req.UserID));
  strncpy(login_req.Password, params.passwd().c_str(), sizeof(login_req.Password));
  if (ctp_api_->ReqUserLogin(&login_req, next_req_id()) != 0) {
    spdlog::error("[CTP] login. Invalid user-login field");
    return false;
  }

  while (!is_login_) {
    if (is_error_) {
      spdlog::error("[CTP] login. Failed to login");
      return false;
    }
  }

  CThostFtdcQrySettlementInfoField qs_req;
  memset(&qs_req, 0, sizeof(qs_req));
  strncpy(qs_req.BrokerID, broker_id_.c_str(), sizeof(qs_req.BrokerID));
  strncpy(qs_req.InvestorID, investor_id_.c_str(), sizeof(qs_req.InvestorID));
  if (ctp_api_->ReqQrySettlementInfo(&qs_req, next_req_id()) != 0) {
    spdlog::error("[CTP] login. ReqQrySettlementInfo returned nonzero");
    return false;
  }

  while (!is_query_settlement_) {
    if (is_error_) {
      spdlog::error("[CTP] login. Failed to ReqQrySettlementInfo");
      return false;
    }
  }

  CThostFtdcSettlementInfoConfirmField confirm_req;
  memset(&confirm_req, 0, sizeof(confirm_req));
  strncpy(confirm_req.BrokerID, broker_id_.c_str(), sizeof(confirm_req.BrokerID));
  strncpy(confirm_req.InvestorID, investor_id_.c_str(), sizeof(confirm_req.InvestorID));
  if (ctp_api_->ReqSettlementInfoConfirm(&confirm_req, next_req_id()) != 0) {
    spdlog::error("[CTP] login. Failed to ReqSettlementInfoConfirm");
    return false;
  }

  while (!is_settlement_confirmed_) {
    if (is_error_) {
      spdlog::error("[CTP] login. Failed to ReqSettlementInfoConfirm");
      return false;
    }
  }

  return true;
}

void CtpTradeApi::OnFrontConnected() {
  is_connected_ = true;
  spdlog::debug("[CTP] OnFrontConnected. Connected to the front {}", front_addr_);
}

void CtpTradeApi::OnFrontDisconnected(int reason) {
  is_error_ = true;
  is_connected_ = false;
  spdlog::error("[CTP] OnFrontDisconnected. Disconnected from the front {}", front_addr_);
}

void CtpTradeApi::OnHeartBeatWarning(int time_lapse) {
  spdlog::warn("[CTP] OnHeartBeatWarning. No packet received for a period of time");
}

void CtpTradeApi::OnRspAuthenticate(
                    CThostFtdcRspAuthenticateField *rsp_authenticate_field,
                    CThostFtdcRspInfoField *rsp_info,
                    int req_id,
                    bool is_last) {
  if (!is_last)
    return;

  if (is_error_rsp(rsp_info)) {
    is_error_ = true;
    return;
  }

  is_authenticated_ = true;
  spdlog::debug("[CTP] OnRspAuthenticate. Investor ID: {}", investor_id_);
}

void CtpTradeApi::OnRspUserLogin(
                    CThostFtdcRspUserLoginField *rsp_user_login,
                    CThostFtdcRspInfoField *rsp_info,
                    int req_id,
                    bool is_last) {
  if (!is_last)
    return;

  if (is_error_rsp(rsp_info)) {
    is_error_ = true;
    spdlog::error("[CTP] OnRspUserLogin. Error ID: {}", rsp_info->ErrorID);
    return;
  }

  front_id_ = rsp_user_login->FrontID;
  session_id_ = rsp_user_login->SessionID;
  int max_order_ref = std::stoi(rsp_user_login->MaxOrderRef);
  next_order_ref_ = max_order_ref + 1;
  is_login_ = true;
  spdlog::debug("[CTP] OnRspUserLogin. Login as {}. "
                "Front ID: {}, Session ID: {}, Max OrderRef: {}",
                investor_id_, front_id_, session_id_, max_order_ref);
}

void CtpTradeApi::OnRspQrySettlementInfo(
                    CThostFtdcSettlementInfoField *settlement_info,
                    CThostFtdcRspInfoField *rsp_info,
                    int req_id,
                    bool is_last) {
  if (!is_last)
    return;

  if (!is_error_rsp(rsp_info)) {
    is_query_settlement_ = true;
    spdlog::debug("[CTP] OnRspQrySettlementInfo. Success");
  } else {
    is_error_ = true;
    spdlog::error("[CTP] OnRspQrySettlementInfo. Error Msg: {}",
                  gb2312_to_utf8(rsp_info->ErrorMsg));
  }
}

void CtpTradeApi::OnRspSettlementInfoConfirm(
        CThostFtdcSettlementInfoConfirmField *settlement_info_confirm,
        CThostFtdcRspInfoField *rsp_info,
        int req_id,
        bool is_last) {
  if (!is_last)
    return;

  if (!is_error_rsp(rsp_info) && settlement_info_confirm) {
    is_settlement_confirmed_ = true;
    spdlog::debug("[CTP] OnRspSettlementInfoConfirm. Settlement info confirmed");
  } else {
    is_error_ = true;
  }
}

std::string CtpTradeApi::send_order(const Order* order) {
  CThostFtdcInputOrderField ctp_order;
  memset(&ctp_order, 0, sizeof(ctp_order));
  strncpy(ctp_order.BrokerID, broker_id_.c_str(), sizeof(ctp_order.BrokerID));
  strncpy(ctp_order.InvestorID, investor_id_.c_str(), sizeof(ctp_order.InvestorID));
  strncpy(ctp_order.ExchangeID, order->exchange.c_str(), sizeof(ctp_order.ExchangeID));
  strncpy(ctp_order.InstrumentID, order->symbol.c_str(), sizeof(ctp_order.InstrumentID));
  snprintf(ctp_order.OrderRef, sizeof(ctp_order.OrderRef), "%d", next_order_ref());
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

  if (ctp_api_->ReqOrderInsert(&ctp_order, next_req_id()) != 0)
    return "";

  return get_order_id(ctp_order.InstrumentID, ctp_order.ExchangeID, ctp_order.OrderRef);
}

void CtpTradeApi::OnRspOrderInsert(
                  CThostFtdcInputOrderField *ctp_order,
                  CThostFtdcRspInfoField *rsp_info,
                  int req_id,
                  bool is_last) {
  if (ctp_order->InvestorID != investor_id_) {
    spdlog::error("[CTP] OnRspOrderInsert. "
                  "= =#Receive RspOrderInsert of other investor");
    return;
  }

  Order order;
  order.order_id = get_order_id(ctp_order->InstrumentID,
                                ctp_order->ExchangeID,
                                ctp_order->OrderRef);
  order.symbol = ctp_order->InstrumentID;
  order.exchange = ctp_order->ExchangeID;
  order.ticker = to_ticker(order.symbol, order.exchange);
  order.direction = direction(ctp_order->Direction);
  order.offset = offset(ctp_order->CombOffsetFlag[0]);
  order.price = ctp_order->LimitPrice;
  order.volume = ctp_order->VolumeTotalOriginal;
  order.type = order_type(ctp_order->OrderPriceType);
  order.status = OrderStatus::REJECTED;

  general_api_->on_order(&order);
}

void CtpTradeApi::OnRtnOrder(CThostFtdcOrderField *ctp_order) {
  // 听说CTP会收到别人的订单回报？判断一下
  if (ctp_order->InvestorID != investor_id_) {
    spdlog::debug("[CTP] OnRtnOrder. Received other investor's RtnOrder");
    return;
  }

  if (!ctp_order) {
    spdlog::error("[CTP] OnRtnOrder. Null pointer of order");
    return;
  }

  Order order;
  order.order_id = get_order_id(ctp_order->InstrumentID,
                                ctp_order->ExchangeID,
                                ctp_order->OrderRef);
  order.symbol = ctp_order->InstrumentID;
  order.exchange = ctp_order->ExchangeID;
  order.ticker = to_ticker(order.symbol, order.exchange);
  order.direction = direction(ctp_order->Direction);
  order.offset = offset(ctp_order->CombOffsetFlag[0]);
  order.price = ctp_order->LimitPrice;
  order.volume_traded = ctp_order->VolumeTraded;
  order.volume = ctp_order->VolumeTotalOriginal;
  order.insert_time = ctp_order->InsertTime;
  order.type = order_type(ctp_order->OrderPriceType);

  if (ctp_order->OrderSubmitStatus == THOST_FTDC_OSS_InsertRejected)
    order.status = OrderStatus::REJECTED;
  else if (ctp_order->OrderSubmitStatus == THOST_FTDC_OSS_CancelRejected)
    order.status = OrderStatus::CANCEL_REJECTED;
  else
    order.status = order_status(ctp_order->OrderStatus);

  spdlog::debug("[CTP] OnRtnOrder. Order ID: {}, Instrument: {}, Exchange: {}, "
                "Direction: {}, Offset: {}, Origin Volume: {}, Traded: {}, "
                "Price: {:.2f}, Status: {}, Status Msg: {}",
                order.order_id, order.symbol, order.exchange, to_string(order.direction),
                to_string(order.offset), order.volume, order.volume_traded, order.price,
                to_string(order.status), gb2312_to_utf8(ctp_order->StatusMsg));

  general_api_->on_order(&order);
}

void CtpTradeApi::OnRtnTrade(CThostFtdcTradeField *trade) {
  if (trade->InvestorID != investor_id_)
    return;

  Trade td;
  td.order_id = get_order_id(trade->InstrumentID, trade->ExchangeID, trade->OrderRef);
  td.symbol = trade->InstrumentID;
  td.exchange = trade->ExchangeID;
  td.ticker = to_ticker(td.symbol, td.exchange);
  td.trade_id = std::stoi(trade->TradeID);
  td.trade_time = trade->TradeTime;
  td.direction = direction(trade->Direction);
  td.offset = offset(trade->OffsetFlag);
  td.price = trade->Price;
  td.volume = trade->Volume;

  spdlog::debug("[CTP] OnRtnTrade. Order ID: {}, Instrument: {}, Exchange: {}, "
              "Trade ID: {}, Trade Time: {}, Direction: {}, Offset: {}, Price: {:.2f}, "
              "Volume: {}",
              td.order_id, td.symbol, td.exchange, td.trade_id, td.trade_time,
              to_string(td.direction), to_string(td.offset), td.price, td.volume);

  general_api_->on_trade(&td);
}

bool CtpTradeApi::cancel_order(const std::string& order_id) {
  CThostFtdcInputOrderActionField req;
  memset(&req, 0, sizeof(req));

  auto fields = split<std::string>(order_id, ".");
  if (fields.size() != 3)
    return false;

  strncpy(req.InstrumentID, fields[0].c_str(), sizeof(req.InstrumentID));
  strncpy(req.ExchangeID, fields[1].c_str(), sizeof(req.ExchangeID));
  strncpy(req.OrderRef, fields[2].c_str(), sizeof(req.OrderRef));
  strncpy(req.BrokerID, broker_id_.c_str(), sizeof(req.BrokerID));
  strncpy(req.InvestorID, investor_id_.c_str(), sizeof(req.InvestorID));
  req.ActionFlag = THOST_FTDC_AF_Delete;
  req.FrontID = front_id_;
  req.SessionID = session_id_;

  if (ctp_api_->ReqOrderAction(&req, next_req_id()) != 0)
    return false;

  return true;
}

void CtpTradeApi::OnRspOrderAction(
                  CThostFtdcInputOrderActionField *action,
                  CThostFtdcRspInfoField *rsp_info,
                  int req_id,
                  bool is_last) {
  if (action->InvestorID != investor_id_)
    return;

  spdlog::error("[CTP] OnRspOrderAction. Rejected.");

  Order order;
  order.order_id = get_order_id(action->InstrumentID, action->ExchangeID, action->OrderRef);
  order.status = OrderStatus::CANCEL_REJECTED;

  general_api_->on_order(&order);
}

bool CtpTradeApi::query_contract(const std::string& symbol, const std::string& exchange) {
  std::unique_lock<std::mutex> lock(query_mutex_);

  CThostFtdcQryInstrumentField req;
  memset(&req, 0, sizeof(req));
  strncpy(req.InstrumentID, symbol.c_str(), sizeof(req.InstrumentID));
  strncpy(req.ExchangeID, exchange.c_str(), sizeof(req.ExchangeID));

  is_qry_contract_done_ = false;
  if (ctp_api_->ReqQryInstrument(&req, next_req_id()) != 0)
    return false;

  while (!is_qry_contract_done_) {
    if (is_error_)
      return false;
  }

  return true;
}

void CtpTradeApi::OnRspQryInstrument(
                    CThostFtdcInstrumentField *instrument,
                    CThostFtdcRspInfoField *rsp_info,
                    int req_id,
                    bool is_last) {
  if (is_error_rsp(rsp_info)) {
    is_error_ = true;
    spdlog::error("[CTP] OnRspQryInstrument. Error Msg: {}",
                  gb2312_to_utf8(rsp_info->ErrorMsg));
    return;
  } else if (!instrument) {
    is_error_ = true;
    spdlog::error("[CTP] OnRspQryInstrument. Null pointer of instrument");
    return;
  }

  spdlog::debug("[CTP] OnRspQryInstrument. Success. Instrument: {}, Exchange: {}",
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



  general_api_->on_contract(&contract);

  if (is_last)
    is_qry_contract_done_ = true;
}

bool CtpTradeApi::query_position(const std::string& symbol,
                                       const std::string& exchange) {
  std::unique_lock<std::mutex> lock(query_mutex_);

  CThostFtdcQryInvestorPositionField req;
  memset(&req, 0, sizeof(req));
  strncpy(req.BrokerID, broker_id_.c_str(), sizeof(req.BrokerID));
  strncpy(req.InvestorID, investor_id_.c_str(), sizeof(req.InvestorID));
  strncpy(req.InstrumentID, symbol.c_str(), symbol.size());
  strncpy(req.ExchangeID, exchange.c_str(), sizeof(req.ExchangeID));

  is_qry_position_done_ = false;
  if (ctp_api_->ReqQryInvestorPosition(&req, next_req_id()) != 0)
    return false;

  while (!is_qry_position_done_) {
    if (is_error_)
      return false;
  }

  return true;
}

void CtpTradeApi::OnRspQryInvestorPosition(
                    CThostFtdcInvestorPositionField *position,
                    CThostFtdcRspInfoField *rsp_info,
                    int req_id,
                    bool is_last) {
  spdlog::debug("[CTP] OnRspQryInvestorPosition");

  if (is_error_rsp(rsp_info)) {
    pos_cache_.clear();
    is_error_ = true;
    spdlog::error("[CTP] OnRspQryInvestorPosition. Error Msg: {}",
                  gb2312_to_utf8(rsp_info->ErrorMsg));
    return;
  }

  if (position) {
    auto ticker = to_ticker(position->InstrumentID, position->ExchangeID);
    auto& pos = pos_cache_[ticker];
    if (pos.ticker.empty()) {
      pos.symbol = position->InstrumentID;
      pos.exchange = position->ExchangeID;
      pos.ticker = ticker;
    }

    bool is_long_pos = position->PosiDirection == THOST_FTDC_PD_Long;
    auto& pos_detail =  is_long_pos ? pos.long_pos : pos.short_pos;
    if (pos.exchange == kSHFE || pos.exchange == kINE)
      pos_detail.yd_volume = position->YdPosition;
    else
      pos_detail.yd_volume = position->Position - position->TodayPosition;

    if (is_long_pos)
      pos_detail.frozen += position->LongFrozen;
    else
      pos_detail.frozen += position->ShortFrozen;

    pos_detail.volume = position->Position;
    pos_detail.pnl = position->PositionProfit;

    auto contract = ContractTable::get_by_ticker(pos.ticker);
    if (!contract)
      spdlog::warn("[CTP] OnRspQryInvestorPosition. {} is not in contract list. "
                    "Please update the contract list before other operations",
                    pos.ticker);
    else if (pos_detail.volume > 0 && contract->size > 0)
      pos_detail.cost_price = position->PositionCost / (pos_detail.volume * contract->size);
  }

  if (is_last) {
    for (auto& [key, pos] : pos_cache_)
      general_api_->on_position(&pos);
    // 查询结束回调空指针作为结束信号
    general_api_->on_position(nullptr);
    pos_cache_.clear();
    is_qry_position_done_ = true;
  }
}

bool CtpTradeApi::query_account() {
  std::unique_lock<std::mutex> lock(query_mutex_);

  CThostFtdcQryTradingAccountField req;
  memset(&req, 0, sizeof(req));
  strncpy(req.BrokerID, broker_id_.c_str(), sizeof(req.BrokerID));
  strncpy(req.InvestorID, investor_id_.c_str(), sizeof(req.InvestorID));

  spdlog::debug("[CTP] query_account");
  is_qry_account_done_ = false;
  if (ctp_api_->ReqQryTradingAccount(&req, next_req_id()) != 0)
    return false;

  while (!is_qry_account_done_) {
    if (is_error_)
      return false;
  }

  return true;
}

void CtpTradeApi::OnRspQryTradingAccount(
                    CThostFtdcTradingAccountField *trading_account,
                    CThostFtdcRspInfoField *rsp_info,
                    int req_id,
                    bool is_last) {
  if (!is_last)
    return;

  if (is_error_rsp(rsp_info)) {
    is_error_ = true;
    return;
  }

  spdlog::debug("[CTP] OnRspQryTradingAccount. Account ID: {}, Balance: {}, Frozen: {}",
                trading_account->AccountID, trading_account->Balance,
                trading_account->FrozenMargin);

  Account account;
  account.account_id = trading_account->AccountID;
  account.balance = trading_account->Balance;
  account.frozen = trading_account->FrozenCash +
                   trading_account->FrozenMargin +
                   trading_account->FrozenCommission;

  general_api_->on_account(&account);
  is_qry_account_done_ = true;
}

}  // namespace ft
