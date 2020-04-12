// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "ctp/CtpGateway.h"

#include <cassert>
#include <cstring>

#include <spdlog/spdlog.h>
#include <ThostFtdcUserApiDataType.h>
#include <ThostFtdcUserApiStruct.h>

#include "Account.h"
#include "ctp/CtpCommon.h"
#include "ctp/CtpTradeSpi.h"
#include "ctp/FieldMapper.h"
#include "Contract.h"
#include "Position.h"
#include "Trade.h"

namespace ft {

CtpGateway::CtpGateway() {
}

CtpGateway::~CtpGateway() {
  if (api_)
    api_->Release();

  if (spi_)
    delete spi_;
}

bool CtpGateway::login(const LoginParams& params) {
  if (params.broker_id().size() > sizeof(TThostFtdcBrokerIDType) ||
      params.investor_id().size() > sizeof(TThostFtdcUserIDType) ||
      params.passwd().size() > sizeof(TThostFtdcPasswordType) ||
      params.auth_code().size() > sizeof(TThostFtdcAuthCodeType) ||
      params.app_id().size() > sizeof(TThostFtdcAppIDType)) {
    spdlog::error("[CTP] Invalid login params");
    return false;
  }

  api_ = CThostFtdcTraderApi::CreateFtdcTraderApi();
  if (!api_) {
    spdlog::error("[CTP] Failed to create CTP API");
    return false;
  }

  api_->SubscribePrivateTopic(THOST_TERT_QUICK);

  front_addr_ = params.front_addr();
  broker_id_ = params.broker_id();
  investor_id_ = params.investor_id();

  spi_ = new CtpTradeSpi(this);
  api_->RegisterSpi(spi_);
  api_->RegisterFront(const_cast<char*>(params.front_addr().c_str()));
  api_->Init();
  while (!is_connected_)
    continue;

  int req_id;
  AsyncStatus status;

  if (!params.auth_code().empty()) {
    CThostFtdcReqAuthenticateField auth_req;
    memset(&auth_req, 0, sizeof(auth_req));
    strncpy(auth_req.BrokerID, params.broker_id().c_str(), sizeof(auth_req.BrokerID));
    strncpy(auth_req.UserID, params.investor_id().c_str(), sizeof(auth_req.UserID));
    strncpy(auth_req.AuthCode, params.auth_code().c_str(), sizeof(auth_req.AuthCode));
    strncpy(auth_req.AppID, params.app_id().c_str(), sizeof(auth_req.AppID));
    req_id = next_req_id();
    status = req_async_status(req_id);
    if (api_->ReqAuthenticate(&auth_req, req_id) != 0)
      rsp_async_status(req_id, false);

    if (!status.wait()) {
      spdlog::error("[CTP] Failed to authenticate");
      return false;
    }
  }

  CThostFtdcReqUserLoginField login_req;
  memset(&login_req, 0, sizeof(login_req));
  strncpy(login_req.BrokerID, params.broker_id().c_str(), sizeof(login_req.BrokerID));
  strncpy(login_req.UserID, params.investor_id().c_str(), sizeof(login_req.UserID));
  strncpy(login_req.Password, params.passwd().c_str(), sizeof(login_req.Password));
  req_id = next_req_id();
  status = req_async_status(req_id);
  if (api_->ReqUserLogin(&login_req, req_id) != 0) {
    spdlog::error("[CTP] Invalid user-login field");
    rsp_async_status(req_id, false);
  }

  if (!status.wait()) {
    spdlog::error("[CTP] Failed to login");
    return false;
  }

  // 投资结算确认后才能进行交易
  status = query_settlement();
  if (!status.wait()) {
    spdlog::error("[CTP] Failed to query settlement info");
    return false;
  }

  status = req_settlement_confirm();
  if (!status.wait()) {
    spdlog::error("[CTP] Failed to confirm settlement info");
    return false;
  }

  is_login_ = true;
  return true;
}

void CtpGateway::on_connected() {
  is_connected_ = true;
  spdlog::debug("[CTP] on_connected. Connected to the front {}", front_addr_);
}

void CtpGateway::on_disconnected(int reason) {
  is_connected_ = false;
  spdlog::error("[CTP] on_disconnected. Disconnected from the front {}", front_addr_);
}

void CtpGateway::on_heart_beat_warning(int time_lapse) {
  spdlog::warn("[CTP] on_heart_beat_warning. No packet received for a period of time");
}

void CtpGateway::on_authenticate(
                    CThostFtdcRspAuthenticateField *rsp_authenticate_field,
                    CThostFtdcRspInfoField *rsp_info,
                    int req_id,
                    bool is_last) {
  if (!is_last)
    return;

  if (is_error_rsp(rsp_info)) {
    rsp_async_status(req_id, false);
    return;
  }

  rsp_async_status(req_id, true);
  spdlog::debug("[CTP] Authenticated. Investor ID: {}", investor_id_);
}

void CtpGateway::on_login(
                    CThostFtdcRspUserLoginField *rsp_user_login,
                    CThostFtdcRspInfoField *rsp_info,
                    int req_id,
                    bool is_last) {
  if (!is_last)
    return;

  if (is_error_rsp(rsp_info)) {
    spdlog::error("[CTP] on_login. Error ID: {}", rsp_info->ErrorID);
    rsp_async_status(req_id, false);
    return;
  }

  front_id_ = rsp_user_login->FrontID;
  session_id_ = rsp_user_login->SessionID;
  int max_order_ref = std::stoi(rsp_user_login->MaxOrderRef);
  next_order_ref_ = max_order_ref + 1;
  rsp_async_status(req_id, true);
  spdlog::info("[CTP] on_login. Login as {}. "
               "Front ID: {}, Session ID: {}, Max OrderRef: {}",
               investor_id_, front_id_, session_id_, max_order_ref);
}

AsyncStatus CtpGateway::query_settlement() {
  CThostFtdcQrySettlementInfoField req;
  memset(&req, 0, sizeof(req));
  strncpy(req.BrokerID, broker_id_.c_str(), sizeof(req.BrokerID));
  strncpy(req.InvestorID, investor_id_.c_str(), sizeof(req.InvestorID));

  int req_id = next_req_id();
  auto status = req_async_status(req_id);
  if (api_->ReqQrySettlementInfo(&req, req_id) != 0) {
    spdlog::error("[CTP] on_login. ReqQrySettlementInfo returned nonzero");
    rsp_async_status(req_id, false);
  }

  spdlog::debug("[CTP] query_settlement");
  return status;
}

void CtpGateway::on_settlement(CThostFtdcSettlementInfoField *settlement_info,
                               CThostFtdcRspInfoField *rsp_info,
                               int req_id,
                               bool is_last) {
  if (!is_last)
    return;

  bool res = false;
  if (!is_error_rsp(rsp_info)) {
    res = true;
    spdlog::debug("[CTP] on_settlement. Success");
  } else {
    spdlog::error("[CTP] on_settlement. Error Msg: {}",
                  gb2312_to_utf8(rsp_info->ErrorMsg));
  }
  rsp_async_status(req_id, res);
}

AsyncStatus CtpGateway::req_settlement_confirm() {
  CThostFtdcSettlementInfoConfirmField req;
  memset(&req, 0, sizeof(req));
  strncpy(req.BrokerID, broker_id_.c_str(), sizeof(req.BrokerID));
  strncpy(req.InvestorID, investor_id_.c_str(), sizeof(req.InvestorID));

  int req_id = next_req_id();
  auto status = req_async_status(req_id);
  if (api_->ReqSettlementInfoConfirm(&req, req_id) != 0)
    rsp_async_status(req_id, false);

  spdlog::debug("[CTP] req_settlement_confirm");
  return status;
}

void CtpGateway::on_settlement_confirm(
        CThostFtdcSettlementInfoConfirmField *settlement_info_confirm,
        CThostFtdcRspInfoField *rsp_info,
        int req_id,
        bool is_last) {
  if (!is_last)
    return;

  bool res = false;
  if (!is_error_rsp(rsp_info) && settlement_info_confirm) {
    res = true;
    spdlog::debug("[CTP] on_settlement_confirm. Settlement info confirmed");
  }
  rsp_async_status(req_id, res);
}

std::string CtpGateway::send_order(const Order* order) {
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

  std::string order_id;
  if (api_->ReqOrderInsert(&ctp_order, next_req_id()) != 0)
    return order_id;

  order_id = get_order_id(ctp_order.OrderRef);

  {
    std::unique_lock<std::mutex> lock(order_mutex_);
    id2order_.emplace(order_id, *order);
  }

  return order_id;
}

void CtpGateway::on_order_rejected(
                  CThostFtdcInputOrderField *ctp_order,
                  CThostFtdcRspInfoField *rsp_info,
                  int req_id,
                  bool is_last) {
  if (ctp_order->InvestorID != investor_id_) {
    spdlog::error("[CTP] on_order_rejected. "
                  "= =#Receive RspOrderInsert of other investor");
    return;
  }

  Order order;
  order.order_id = get_order_id(ctp_order->OrderRef);
  order.symbol = ctp_order->InstrumentID;
  order.exchange = ctp_order->ExchangeID;
  order.ticker = to_ticker(order.symbol, order.exchange);
  order.direction = direction(ctp_order->Direction);
  order.offset = offset(ctp_order->CombOffsetFlag[0]);
  order.price = ctp_order->LimitPrice;
  order.volume = ctp_order->VolumeTotalOriginal;
  order.type = order_type(ctp_order->OrderPriceType);
  order.status = OrderStatus::REJECTED;

  spdlog::error("[CTP] on_order_rejected. Order's failed to submit. "
                "Order ID: {}, Instrument: {}, Exchange: {}, Error Msg: {}",
                order.order_id, order.symbol, order.exchange,
                gb2312_to_utf8(rsp_info->ErrorMsg));
  trader_->on_order(&order);
}

void CtpGateway::on_order(CThostFtdcOrderField *ctp_order) {
  // 听说CTP会收到别人的订单回报？判断一下
  if (ctp_order->InvestorID != investor_id_) {
    spdlog::debug("[CTP] on_order. Received other investor's RtnOrder");
    return;
  }

  if (!ctp_order) {
    spdlog::error("[CTP] on_order. Null pointer of order");
    return;
  }

  Order order;
  order.order_id = get_order_id(ctp_order->OrderRef);
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

  spdlog::debug("[CTP] on_order. Order ID: {}, Instrument: {}, Exchange: {}, "
                "Direction: {}, Offset: {}, Origin Volume: {}, Traded: {}, "
                "Price: {}, Status: {}, Status Msg: {}",
                order.order_id, order.symbol, order.exchange, to_string(order.direction),
                to_string(order.offset), order.volume, order.volume_traded, order.price,
                to_string(order.status), gb2312_to_utf8(ctp_order->StatusMsg));

  if (order.status == OrderStatus::REJECTED ||
      order.status == OrderStatus::CANCELED ||
      order.status == OrderStatus::ALL_TRADED) {
    std::unique_lock<std::mutex> lock(order_mutex_);
    id2order_.erase(order.order_id);
  }

  trader_->on_order(&order);
}

void CtpGateway::on_trade(CThostFtdcTradeField *trade) {
  if (trade->InvestorID != investor_id_)
    return;

  Trade td;
  td.order_id = get_order_id(trade->OrderRef);
  td.symbol = trade->InstrumentID;
  td.exchange = trade->ExchangeID;
  td.ticker = to_ticker(td.symbol, td.exchange);
  td.trade_id = trade->TradeID;
  td.trade_time = trade->TradeTime;
  td.direction = direction(trade->Direction);
  td.offset = offset(trade->OffsetFlag);
  td.price = trade->Price;
  td.volume = trade->Volume;

  spdlog::debug("[CTP] on_trade. Order ID: {}, Instrument: {}, Exchange: {}, "
              "Trade ID: {}, Trade Time: {}, Direction: {}, Offset: {}, Price: {}, "
              "Volume: {}",
              td.order_id, td.symbol, td.exchange, td.trade_id, td.trade_time,
              to_string(td.direction), to_string(td.offset), td.price, td.volume);

  trader_->on_trade(&td);
}

bool CtpGateway::cancel_order(const std::string& order_id) {
  CThostFtdcInputOrderActionField req;
  memset(&req, 0, sizeof(req));

  {
    std::unique_lock<std::mutex> lock(order_mutex_);
    auto iter = id2order_.find(order_id);
    if (iter == id2order_.end())
      return false;

    const auto& order = iter->second;
    strncpy(req.InstrumentID, order.symbol.c_str(), sizeof(req.InstrumentID));
    strncpy(req.ExchangeID, order.exchange.c_str(), sizeof(req.ExchangeID));
  }

  strncpy(req.BrokerID, broker_id_.c_str(), sizeof(req.BrokerID));
  strncpy(req.InvestorID, investor_id_.c_str(), sizeof(req.InvestorID));
  req.ActionFlag = THOST_FTDC_AF_Delete;

  char *end;
  req.FrontID = strtol(order_id.c_str(), &end, 0);
  req.SessionID = strtol(end, nullptr, 0);
  auto pos = order_id.find_last_of('_') + 1;
  strncpy(req.OrderRef, &order_id[pos], sizeof(req.OrderRef));

  if (api_->ReqOrderAction(&req, next_req_id()) != 0)
    return false;

  return true;
}

void CtpGateway::on_order_action(
                  CThostFtdcInputOrderActionField *action,
                  CThostFtdcRspInfoField *rsp_info,
                  int req_id,
                  bool is_last) {
  if (action->InvestorID != investor_id_)
    return;

  spdlog::error("[CTP] on_order_action. Rejected.");

  std::unique_lock<std::mutex> lock(order_mutex_);
  auto order = id2order_[get_order_id(action->OrderRef)];
  lock.unlock();
  order.status = OrderStatus::CANCEL_REJECTED;

  trader_->on_order(&order);
}

AsyncStatus CtpGateway::query_contract(const std::string& symbol,
                                       const std::string& exchange) {
  CThostFtdcQryInstrumentField req;
  memset(&req, 0, sizeof(req));
  if (!symbol.empty()) {
    strncpy(req.InstrumentID, symbol.c_str(), sizeof(req.InstrumentID));
    strncpy(req.ExchangeID, exchange.c_str(), sizeof(req.ExchangeID));
  }

  int req_id = next_req_id();
  auto status = req_async_status(req_id);
  if (api_->ReqQryInstrument(&req, req_id) != 0)
    rsp_async_status(req_id, false);
  return status;
}

void CtpGateway::on_contract(
                    CThostFtdcInstrumentField *instrument,
                    CThostFtdcRspInfoField *rsp_info,
                    int req_id,
                    bool is_last) {
  {
    std::unique_lock<std::mutex> lock(status_mutex_);
    if (req_status_.find(req_id) == req_status_.end()) {
      spdlog::warn("[CTP] on_contract. Invalid rsp");
      return;
    }
  }

  if (is_error_rsp(rsp_info)) {
    rsp_async_status(req_id, false);
    spdlog::error("[CTP] on_contract. Error Msg: {}", gb2312_to_utf8(rsp_info->ErrorMsg));
    return;
  } else if (!instrument) {
    rsp_async_status(req_id, true);
    spdlog::error("[CTP] on_contract. Null pointer of instrument");
    return;
  }

  spdlog::debug("[CTP] on_contract. Success. Instrument: {}, Exchange: {}",
              instrument->InstrumentID, instrument->ExchangeID);

  Contract contract;
  contract.product_type = product_type(instrument->ProductClass);
  contract.symbol = instrument->InstrumentID;
  contract.exchange = instrument->ExchangeID;
  contract.ticker = to_ticker(contract.symbol, contract.exchange);
  contract.name = instrument->InstrumentName;
  contract.size = instrument->VolumeMultiple;
  contract.price_tick = instrument->PriceTick;

  trader_->on_contract(&contract);

  if (is_last)
    rsp_async_status(req_id, true);
}

AsyncStatus CtpGateway::query_position(const std::string& symbol,
                                       const std::string& exchange) {
  CThostFtdcQryInvestorPositionField req;
  memset(&req, 0, sizeof(req));
  strncpy(req.BrokerID, broker_id_.c_str(), sizeof(req.BrokerID));
  strncpy(req.InvestorID, investor_id_.c_str(), sizeof(req.InvestorID));
  strncpy(req.InstrumentID, symbol.c_str(), symbol.size());
  strncpy(req.ExchangeID, exchange.c_str(), sizeof(req.ExchangeID));

  int req_id = next_req_id();
  auto status = req_async_status(req_id);
  if (api_->ReqQryInvestorPosition(&req, req_id) != 0)
    status.set_error();
  return status;
}

void CtpGateway::on_position(
                    CThostFtdcInvestorPositionField *position,
                    CThostFtdcRspInfoField *rsp_info,
                    int req_id,
                    bool is_last) {
  {
    std::unique_lock<std::mutex> lock(status_mutex_);
    if (req_status_.find(req_id) == req_status_.end()) {
      spdlog::warn("[CTP] on_position. Invalid rsp");
      return;
    }
  }

  spdlog::debug("[CTP] on_position");
  auto& pos_cache = pos_caches_[req_id];

  if (is_error_rsp(rsp_info)) {
    pos_caches_.erase(req_id);
    rsp_async_status(req_id, false);
    spdlog::error("[CTP] on_position. Error Msg: {}", gb2312_to_utf8(rsp_info->ErrorMsg));
    return;
  }

  if (position) {
    std::string key = fmt::format("{}.{}_{}",
                                  position->InstrumentID,
                                  position->ExchangeID,
                                  position->PosiDirection);
    auto iter = pos_cache.find(key);
    if (iter == pos_cache.end()) {
      Position pos(position->InstrumentID,
                   position->ExchangeID,
                   direction(position->PosiDirection));
      auto pair = pos_cache.emplace(std::move(key), std::move(pos));
      iter = pair.first;
    }
    auto& pos = iter->second;
    if (pos.exchange == kSHFE || pos.exchange == kINE)
      pos.yd_volume = position->YdPosition;
    else
      pos.yd_volume = position->Position - position->TodayPosition;

    auto ticker = to_ticker(position->InstrumentID, position->ExchangeID);
    auto contract = ContractTable::get(ticker);
    if (!contract) {
      spdlog::warn("[CTP] on_position. {} is not in interest list. "
                    "Please query the contract before other operations",
                    ticker);
      return;
    }

    int size = contract->size;
    double cost = pos.price * pos.volume * size;

    pos.volume += position->Position;
    pos.pnl += position->PositionProfit;

    if (pos.volume > 0 && size > 0) {
      cost += position->PositionCost;
      pos.price = cost / (pos.volume * size);
    }

    if (pos.direction == Direction::BUY)
      pos.frozen += position->LongFrozen;
    else
      pos.frozen += position->ShortFrozen;
  }

  if (is_last) {
    for (auto& [key, pos] : pos_cache)
      trader_->on_position(&pos);
    pos_caches_.erase(req_id);
    rsp_async_status(req_id, true);
  }
}

AsyncStatus CtpGateway::query_account() {
  CThostFtdcQryTradingAccountField req;
  memset(&req, 0, sizeof(req));
  strncpy(req.BrokerID, broker_id_.c_str(), sizeof(req.BrokerID));
  strncpy(req.InvestorID, investor_id_.c_str(), sizeof(req.InvestorID));

  int req_id = next_req_id();
  auto status = req_async_status(req_id);
  spdlog::debug("[CTP] query_account");
  if (api_->ReqQryTradingAccount(&req, req_id) != 0)
    rsp_async_status(req_id, false);

  return status;
}

void CtpGateway::on_account(
                    CThostFtdcTradingAccountField *trading_account,
                    CThostFtdcRspInfoField *rsp_info,
                    int req_id,
                    bool is_last) {
  if (!is_last)
    return;

  if (is_error_rsp(rsp_info)) {
    rsp_async_status(req_id, false);
    return;
  }

  spdlog::debug("[CTP] on_account. Account ID: {}, Balance: {}, "
                "Frozen: {}",
                trading_account->AccountID, trading_account->Balance,
                trading_account->FrozenMargin);

  Account account;
  account.account_id = trading_account->AccountID;
  account.balance = trading_account->Balance;
  account.frozen = trading_account->FrozenCash +
                   trading_account->FrozenMargin +
                   trading_account->FrozenCommission;

  trader_->on_account(&account);
  rsp_async_status(req_id, true);
}

}  // namespace ft
