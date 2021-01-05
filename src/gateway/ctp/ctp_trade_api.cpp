// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "gateway/ctp/ctp_trade_api.h"

#include <ThostFtdcTraderApi.h>
#include <spdlog/spdlog.h>

#include "utils/misc.h"

namespace ft {

CtpTradeApi::CtpTradeApi(BaseOrderManagementSystem *oms)
    : oms_(oms), trade_api_(CThostFtdcTraderApi::CreateFtdcTraderApi()) {
  if (!trade_api_) {
    spdlog::error("[CtpTradeApi::CtpTradeApi] Failed to CreateFtdcTraderApi");
    exit(-1);
  }
}

CtpTradeApi::~CtpTradeApi() {
  error();
  Logout();
}

bool CtpTradeApi::Login(const Config &config) {
  front_addr_ = config.trade_server_address;
  broker_id_ = config.broker_id;
  investor_id_ = config.investor_id;

  trade_api_->SubscribePrivateTopic(THOST_TERT_QUICK);
  trade_api_->RegisterSpi(this);
  trade_api_->RegisterFront(const_cast<char *>(config.trade_server_address.c_str()));
  trade_api_->Init();
  while (!is_connected_) {
    if (is_error_) {
      spdlog::error("[CtpTradeApi::Login] Failed. Cannot connect to {}", front_addr_);
      return false;
    }
  }

  if (!config.auth_code.empty()) {
    CThostFtdcReqAuthenticateField auth_req{};
    strncpy(auth_req.BrokerID, config.broker_id.c_str(), sizeof(auth_req.BrokerID));
    strncpy(auth_req.UserID, config.investor_id.c_str(), sizeof(auth_req.UserID));
    strncpy(auth_req.AuthCode, config.auth_code.c_str(), sizeof(auth_req.AuthCode));
    strncpy(auth_req.AppID, config.app_id.c_str(), sizeof(auth_req.AppID));

    if (trade_api_->ReqAuthenticate(&auth_req, next_req_id()) != 0) {
      spdlog::error("[CtpTradeApi::Login] Failed. Failed to ReqAuthenticate");
      return false;
    }
    if (!wait_sync()) {
      spdlog::error("[CtpTradeApi::Login] Failed. Failed to authenticate");
      return false;
    }
  }

  CThostFtdcReqUserLoginField login_req{};
  strncpy(login_req.BrokerID, config.broker_id.c_str(), sizeof(login_req.BrokerID));
  strncpy(login_req.UserID, config.investor_id.c_str(), sizeof(login_req.UserID));
  strncpy(login_req.Password, config.password.c_str(), sizeof(login_req.Password));

  if (trade_api_->ReqUserLogin(&login_req, next_req_id()) != 0) {
    spdlog::error("[CtpTradeApi::Login] Failed. Failed to ReqUserLogin");
    return false;
  }
  if (!wait_sync()) {
    spdlog::error("[CtpTradeApi::Login] Failed. Failed to Login");
    return false;
  }

  CThostFtdcQrySettlementInfoField settlement_req{};
  strncpy(settlement_req.BrokerID, broker_id_.c_str(), sizeof(settlement_req.BrokerID));
  strncpy(settlement_req.InvestorID, investor_id_.c_str(), sizeof(settlement_req.InvestorID));

  if (trade_api_->ReqQrySettlementInfo(&settlement_req, next_req_id()) != 0) {
    spdlog::error("[CtpTradeApi::Login] Failed. Failed to ReqQrySettlementInfo");
    return false;
  }
  if (!wait_sync()) {
    spdlog::error("[CtpTradeApi::Login] Failed. Failed to query settlement");
    return false;
  }

  CThostFtdcSettlementInfoConfirmField confirm_req{};
  strncpy(confirm_req.BrokerID, broker_id_.c_str(), sizeof(confirm_req.BrokerID));
  strncpy(confirm_req.InvestorID, investor_id_.c_str(), sizeof(confirm_req.InvestorID));

  if (trade_api_->ReqSettlementInfoConfirm(&confirm_req, next_req_id()) != 0) {
    spdlog::error("[CtpTradeApi::Login] Failed. Failed to ReqSettlementInfoConfirm");
    return false;
  }
  if (!wait_sync()) {
    spdlog::error("[CtpTradeApi::Login] Failed. Failed to confirm settlement info");
    return false;
  }

  if (config.cancel_outstanding_orders_on_startup) {
    spdlog::debug("[CtpTradeApi::Login] Cancel outstanding orders on startup");

    CThostFtdcQryOrderField req{};
    strncpy(req.BrokerID, broker_id_.c_str(), sizeof(req.BrokerID));
    strncpy(req.InvestorID, investor_id_.c_str(), sizeof(req.InvestorID));

    if (trade_api_->ReqQryOrder(&req, next_req_id()) != 0) {
      spdlog::error("[CtpTradeApi::Login] Failed. Failed to ReqQryOrder");
      return false;
    }
    if (!wait_sync()) {
      spdlog::error("[CtpTradeApi::Login] Failed to query orders");
      return false;
    }

    std::this_thread::sleep_for(std::chrono::seconds(1));
  }

  is_logon_ = true;
  return true;
}

void CtpTradeApi::Logout() {
  if (is_logon_) {
    CThostFtdcUserLogoutField req{};
    strncpy(req.BrokerID, broker_id_.c_str(), sizeof(req.BrokerID));
    strncpy(req.UserID, investor_id_.c_str(), sizeof(req.UserID));
    if (trade_api_->ReqUserLogout(&req, next_req_id()) != 0) return;

    while (is_logon_) continue;
  }
}

void CtpTradeApi::OnFrontConnected() {
  spdlog::debug("[CtpTradeApi::OnFrontConnected] Success. Connected to {}", front_addr_);
  is_error_ = false;
  is_connected_ = true;
}

void CtpTradeApi::OnFrontDisconnected(int reason) {
  spdlog::error("[CtpTradeApi::OnFrontDisconnected] . Disconnected from {}", front_addr_);
  exit(-1);
}

void CtpTradeApi::OnHeartBeatWarning(int time_lapse) {
  spdlog::warn("[CtpTradeApi::OnHeartBeatWarning] No packet received for some time");
}

void CtpTradeApi::OnRspAuthenticate(CThostFtdcRspAuthenticateField *rsp_authenticate_field,
                                    CThostFtdcRspInfoField *rsp_info, int req_id, bool is_last) {
  if (!is_last) return;

  if (is_error_rsp(rsp_info)) {
    spdlog::error("[CtpTradeApi::OnRspAuthenticate] Failed. ErrorMsg: {}",
                  gb2312_to_utf8(rsp_info->ErrorMsg));
    error();
    return;
  }

  spdlog::debug("[CTP::OnRspAuthenticate] Success. Investor ID: {}", investor_id_);
  done();
}

void CtpTradeApi::OnRspUserLogin(CThostFtdcRspUserLoginField *rsp_user_login,
                                 CThostFtdcRspInfoField *rsp_info, int req_id, bool is_last) {
  if (!is_last) return;

  if (is_error_rsp(rsp_info)) {
    spdlog::error("[CtpTradeApi::OnRspUserLogin] Failed. ErrorMsg: {}",
                  gb2312_to_utf8(rsp_info->ErrorMsg));
    error();
    return;
  }

  front_id_ = rsp_user_login->FrontID;
  session_id_ = rsp_user_login->SessionID;
  auto max_order_ref = std::stoul(rsp_user_login->MaxOrderRef);
  order_ref_base_ = max_order_ref + 1;

  spdlog::debug(
      "[CtpTradeApi::OnRspUserLogin] Success. Login as {}. "
      "Front ID: {}, Session ID: {}, Max OrderRef: {}",
      investor_id_, front_id_, session_id_, max_order_ref);
  done();
}

void CtpTradeApi::OnRspQrySettlementInfo(CThostFtdcSettlementInfoField *settlement_info,
                                         CThostFtdcRspInfoField *rsp_info, int req_id,
                                         bool is_last) {
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
    CThostFtdcSettlementInfoConfirmField *settlement_info_confirm, CThostFtdcRspInfoField *rsp_info,
    int req_id, bool is_last) {
  if (!is_last) return;

  if (is_error_rsp(rsp_info)) {
    spdlog::debug("[CtpTradeApi::OnRspSettlementInfoConfirm] Failed. ErrorMsg: {}",
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
                                  CThostFtdcRspInfoField *rsp_info, int req_id, bool is_last) {
  spdlog::debug("[CtpTradeApi::OnRspUserLogout] Success. Broker ID: {}, Investor ID: {}",
                user_logout->BrokerID, user_logout->UserID);
  is_logon_ = false;
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
      order.flags & OrderFlag::HEDGE ? THOST_FTDC_HF_Hedge : THOST_FTDC_HF_Speculation;
  req.ContingentCondition = THOST_FTDC_CC_Immediately;
  req.ForceCloseReason = THOST_FTDC_FCC_NotForceClose;
  req.MinVolume = 1;
  req.IsAutoSuspend = 0;
  req.UserForceClose = 0;

  if (order.type == OrderType::FAK) {
    req.TimeCondition = THOST_FTDC_TC_IOC;
    req.VolumeCondition = THOST_FTDC_VC_AV;
  } else if (order.type == OrderType::FOK) {
    req.TimeCondition = THOST_FTDC_TC_IOC;
    req.VolumeCondition = THOST_FTDC_VC_CV;
  } else if (order.type == OrderType::LIMIT) {
    req.TimeCondition = THOST_FTDC_TC_GFD;
    req.VolumeCondition = THOST_FTDC_VC_AV;
  } else if (order.type == OrderType::MARKET) {
    req.TimeCondition = THOST_FTDC_TC_IOC;
    req.VolumeCondition = THOST_FTDC_VC_AV;
    req.LimitPrice = 0.0;
  }

  if (trade_api_->ReqOrderInsert(&req, next_req_id()) != 0) {
    spdlog::error("[CtpTradeApi::SendOrder] Failed to call ReqOrderInsert");
    return false;
  }

  spdlog::debug(
      "[CtpTradeApi::SendOrder] 订单发送成功. OrderID: {}, {}, {}, {}{}"
      "Volume:{}, Price:{:.3f}",
      order.order_id, order_ref, contract->ticker, DirectionToStr(order.direction),
      OffsetToStr(order.offset), order.volume, order.price);
  *privdata_ptr = static_cast<uint64_t>(order.contract->tid);
  return true;
}

void CtpTradeApi::OnRspOrderInsert(CThostFtdcInputOrderField *order,
                                   CThostFtdcRspInfoField *rsp_info, int req_id, bool is_last) {
  if (!order) {
    spdlog::warn("[CtpTradeApi::OnRspOrderInsert] nullptr");
    return;
  }

  if (order->InvestorID != investor_id_) {
    spdlog::warn(
        "[CtpTradeApi::OnRspOrderInsert] Failed. "
        "= =#Receive RspOrderInsert of other investor");
    return;
  }

  uint64_t order_ref;
  try {
    order_ref = std::stoul(order->OrderRef);
  } catch (...) {
    spdlog::error("[CtpTradeApi::OnRspOrderInsert] Invalid order ref");
    return;
  }

  OrderRejection rsp{get_order_id(order_ref), gb2312_to_utf8(rsp_info->ErrorMsg)};
  oms_->OnOrderRejected(&rsp);
}

void CtpTradeApi::OnRtnOrder(CThostFtdcOrderField *order) {
  if (!order) {
    spdlog::warn("[CtpTradeApi::OnRtnOrder] nullptr");
    return;
  }

  // 听说CTP会收到别人的订单回报？判断一下
  if (order->InvestorID != investor_id_) {
    spdlog::warn("[CtpTradeApi::OnRtnOrder] Failed. Unknown order");
    return;
  }

  // CTP返回的OrderRef不会有问题吧？
  uint64_t order_id = get_order_id(std::stoul(order->OrderRef));

  // 被拒单或撤销被拒，回调相应函数
  if (order->OrderSubmitStatus == THOST_FTDC_OSS_InsertRejected) {
    OrderRejection rsp{order_id, gb2312_to_utf8(order->StatusMsg)};
    oms_->OnOrderRejected(&rsp);
    return;
  } else if (order->OrderSubmitStatus == THOST_FTDC_OSS_CancelRejected) {
    OrderCancelRejection rsp = {order_id, gb2312_to_utf8(order->StatusMsg)};
    oms_->OnOrderCancelRejected(&rsp);
    return;
  } else if (order->OrderSubmitStatus == THOST_FTDC_OSS_InsertSubmitted ||
             order->OrderSubmitStatus == THOST_FTDC_OSS_CancelSubmitted) {
    return;
  }

  // 如果只是被CTP接收，则直接返回，只能撤被交易所接受的单
  if (order->OrderStatus == THOST_FTDC_OST_Unknown ||
      order->OrderStatus == THOST_FTDC_OST_NoTradeNotQueueing)
    return;

  // 处理撤单
  if (order->OrderStatus == THOST_FTDC_OST_PartTradedNotQueueing ||
      order->OrderStatus == THOST_FTDC_OST_Canceled) {
    OrderCancellation rsp = {order_id, order->VolumeTotalOriginal - order->VolumeTraded};
    oms_->OnOrderCanceled(&rsp);
  } else if (order->OrderStatus == THOST_FTDC_OST_NoTradeQueueing) {
    auto contract = ContractTable::get_by_ticker(order->InstrumentID);
    assert(contract);
    OrderAcceptance rsp = {order_id};
    oms_->OnOrderAccepted(&rsp);
    printf("onrtnorder:  orderref:%s\n", order->OrderRef);
  }
}

void CtpTradeApi::OnRtnTrade(CThostFtdcTradeField *trade) {
  if (!trade) {
    spdlog::warn("[CtpTradeApi::OnRtnTrade] nullptr");
    return;
  }

  if (trade->InvestorID != investor_id_) {
    spdlog::warn("[CtpTradeApi::OnRtnTrade] Failed. Recv unknown trade");
    return;
  }

  auto contract = ContractTable::get_by_ticker(trade->InstrumentID);
  assert(contract);

  Trade rsp{};
  rsp.order_id = get_order_id(std::stoul(trade->OrderRef));
  rsp.volume = trade->Volume;
  rsp.price = trade->Price;
  rsp.trade_type = TradeType::SECONDARY_MARKET;
  oms_->OnOrderTraded(&rsp);
}

bool CtpTradeApi::CancelOrder(uint64_t order_id, uint64_t tid) {
  auto contract = ContractTable::get_by_index(tid);
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
    spdlog::error("[CtpTradeApi::CancelOrder] Failed to ReqOrderAction");
    return false;
  }

  return true;
}

void CtpTradeApi::OnRspOrderAction(CThostFtdcInputOrderActionField *action,
                                   CThostFtdcRspInfoField *rsp_info, int req_id, bool is_last) {
  if (!action) spdlog::warn("[CtpTradeApi::OnRspOrderAction] nullptr");

  if (action->InvestorID != investor_id_) {
    spdlog::warn("[CtpTradeApi::OnRspOrderAction] Recv unknown action");
    return;
  }

  spdlog::error("[CtpTradeApi::OnRspOrderAction] Rejected. Reason: {}",
                gb2312_to_utf8(rsp_info->ErrorMsg));
}

bool CtpTradeApi::QueryContractList(std::vector<Contract> *result) {
  contract_results_ = result;
  CThostFtdcQryInstrumentField req{};
  if (trade_api_->ReqQryInstrument(&req, next_req_id()) != 0) {
    spdlog::error("[CtpTradeApi::QueryContract] Failed to ReqQryInstrument");
    return false;
  }

  return wait_sync();
}

void CtpTradeApi::OnRspQryInstrument(CThostFtdcInstrumentField *instrument,
                                     CThostFtdcRspInfoField *rsp_info, int req_id, bool is_last) {
  if (is_error_rsp(rsp_info)) {
    spdlog::error("[CtpTradeApi::OnRspQryInstrument] Error Msg: {}",
                  gb2312_to_utf8(rsp_info->ErrorMsg));
    error();
    return;
  }

  if (!instrument) {
    spdlog::error("[CtpTradeApi::OnRspQryInstrument] Instrument is nullptr");
    error();
    return;
  }

  spdlog::debug("[CtpTradeApi::OnRspQryInstrument] Instrument: {}, Exchange: {}, {}",
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

  contract_results_->emplace_back(contract);

  if (is_last) done();
}

bool CtpTradeApi::QueryPositionList(std::vector<Position> *result) {
  position_results_ = result;
  CThostFtdcQryInvestorPositionField req{};
  strncpy(req.BrokerID, broker_id_.c_str(), sizeof(req.BrokerID));
  strncpy(req.InvestorID, investor_id_.c_str(), sizeof(req.InvestorID));

  if (trade_api_->ReqQryInvestorPosition(&req, next_req_id()) != 0) {
    spdlog::error("[CtpTradeApi::QueryPosition] Failed to send query req");
    return false;
  }

  return wait_sync();
}

void CtpTradeApi::OnRspQryInvestorPosition(CThostFtdcInvestorPositionField *position,
                                           CThostFtdcRspInfoField *rsp_info, int req_id,
                                           bool is_last) {
  if (is_error_rsp(rsp_info)) {
    spdlog::error("[CtpTradeApi::OnRspQryInvestorPosition] Error Msg: {}",
                  gb2312_to_utf8(rsp_info->ErrorMsg));
    pos_cache_.clear();
    error();
    return;
  }

  if (position) {
    auto contract = ContractTable::get_by_ticker(position->InstrumentID);
    if (!contract) {
      spdlog::error("[CtpTradeApi::OnRspQryInvestorPosition] Contract not found: {}",
                    position->InstrumentID);
      goto check_last;
    }

    auto &pos = pos_cache_[contract->tid];
    pos.tid = contract->tid;

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

    spdlog::debug(
        "[CtpTradeApi::OnRspQryInvestorPosition] {}, long:{}, ydlong:{}, "
        "short:{}, ydshort:{}",
        contract->ticker, pos.long_pos.holdings, pos.long_pos.yd_holdings, pos.short_pos.holdings,
        pos.short_pos.yd_holdings);
  }

check_last:
  if (is_last) {
    for (auto &[tid, pos] : pos_cache_) {
      UNUSED(tid);
      if (position_results_) position_results_->emplace_back(pos);
    }
    pos_cache_.clear();
    done();
  }
}

bool CtpTradeApi::QueryAccount(Account *account) {
  account_result_ = account;
  CThostFtdcQryTradingAccountField req{};
  strncpy(req.BrokerID, broker_id_.c_str(), sizeof(req.BrokerID));
  strncpy(req.InvestorID, investor_id_.c_str(), sizeof(req.InvestorID));

  if (trade_api_->ReqQryTradingAccount(&req, next_req_id()) != 0) {
    spdlog::error("[CtpTradeApi::QueryAccount] Failed to ReqQryTradingAccount");
    return false;
  }

  return wait_sync();
}

void CtpTradeApi::OnRspQryTradingAccount(CThostFtdcTradingAccountField *trading_account,
                                         CThostFtdcRspInfoField *rsp_info, int req_id,
                                         bool is_last) {
  if (!is_last) return;

  if (is_error_rsp(rsp_info)) {
    spdlog::error("[CtpTradeApi::OnRspQryTradingAccount] ErrorMsg: {}",
                  gb2312_to_utf8(rsp_info->ErrorMsg));
    error();
    return;
  }

  spdlog::debug(
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

  if (account_result_) *account_result_ = account;
  done();
}

void CtpTradeApi::OnRspQryOrder(CThostFtdcOrderField *order, CThostFtdcRspInfoField *rsp_info,
                                int req_id, bool is_last) {
  // TODO(kevin)

  if (is_error_rsp(rsp_info)) {
    spdlog::error("[CtpTradeApi::OnRspQryOrder] ErrorMsg: {}", gb2312_to_utf8(rsp_info->ErrorMsg));
    error();
    return;
  }

  if (order && (order->OrderStatus == THOST_FTDC_OST_NoTradeQueueing ||
                order->OrderStatus == THOST_FTDC_OST_PartTradedQueueing)) {
    spdlog::info(
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
      spdlog::error("[CtpTradeApi::OnRspQryOrder] Failed to call ReqOrderAction");
  }

  if (is_last) done();
}

bool CtpTradeApi::QueryTradeList(std::vector<Trade> *result) {
  trade_results_ = result;
  CThostFtdcQryTradeField req{};
  strncpy(req.BrokerID, broker_id_.c_str(), sizeof(req.BrokerID));
  strncpy(req.InvestorID, investor_id_.c_str(), sizeof(req.InvestorID));

  if (trade_api_->ReqQryTrade(&req, next_req_id()) != 0) {
    spdlog::error("[CtpTradeApi::QueryTradeList] Failed. Failed to ReqQryTrade");
    return false;
  }

  return wait_sync();
}

void CtpTradeApi::OnRspQryTrade(CThostFtdcTradeField *trade, CThostFtdcRspInfoField *rsp_info,
                                int req_id, bool is_last) {
  // TODO(kevin)
  if (!is_last) return;

  if (is_error_rsp(rsp_info)) {
    spdlog::error("[CtpTradeApi::OnRspQryTrade] Failed. ErrorMsg: {}",
                  gb2312_to_utf8(rsp_info->ErrorMsg));
    error();
    return;
  }

  if (trade) {
    auto contract = ContractTable::get_by_ticker(trade->InstrumentID);
    assert(contract);

    Trade td{};
    td.tid = contract->tid;
    td.volume = trade->Volume;
    td.price = trade->Price;
    td.direction = direction(trade->Direction);
    td.offset = offset(trade->OffsetFlag);

    if (trade_results_) trade_results_->emplace_back(td);
  }

  if (is_last) done();
}

bool CtpTradeApi::QueryMarginRate(const std::string &ticker) {
  CThostFtdcQryInstrumentMarginRateField req{};

  if (!ticker.empty()) {
    auto contract = ContractTable::get_by_ticker(ticker);
    if (!contract) {
      spdlog::error("[CtpTradeApi::QueryMarginRate] Contract not found. Ticker: {}", ticker);
      return false;
    }
    strncpy(req.InstrumentID, contract->ticker.c_str(), sizeof(req.InstrumentID));
    strncpy(req.ExchangeID, contract->exchange.c_str(), sizeof(req.ExchangeID));
  }

  req.HedgeFlag = THOST_FTDC_HF_Speculation;
  strncpy(req.BrokerID, broker_id_.c_str(), sizeof(req.BrokerID));
  strncpy(req.InvestorID, investor_id_.c_str(), sizeof(req.InvestorID));

  if (trade_api_->ReqQryInstrumentMarginRate(&req, next_req_id()) != 0) {
    spdlog::error(
        "[CtpTradeApi::QueryMarginRate] Failed. "
        "Failed to call ReqQryInstrumentMarginRate");
    return false;
  }

  return wait_sync();
}

void CtpTradeApi::OnRspQryInstrumentMarginRate(CThostFtdcInstrumentMarginRateField *margin_rate,
                                               CThostFtdcRspInfoField *rsp_info, int req_id,
                                               bool is_last) {
  if (is_error_rsp(rsp_info)) {
    spdlog::error("[CtpTradeApi::OnRspQryInstrumentMarginRate] Failed. ErrorMsg: {}",
                  gb2312_to_utf8(rsp_info->ErrorMsg));
    error();
    return;
  }

  if (margin_rate) {
    // TODO(kevin)
    auto __c = ContractTable::get_by_ticker(margin_rate->InstrumentID);
    if (!__c) {
      spdlog::error("[CtpTradeApi::OnRspQryInstrumentMarginRate] Contract not found: {}",
                    margin_rate->InstrumentID);
    }

    spdlog::info("Margin Rate. {}, {}, {}", margin_rate->InstrumentID,
                 margin_rate->LongMarginRatioByMoney, margin_rate->ShortMarginRatioByMoney);

    auto contract = const_cast<Contract *>(__c);
    contract->long_margin_rate = margin_rate->LongMarginRatioByMoney;
    contract->short_margin_rate = margin_rate->ShortMarginRatioByMoney;
  }

  if (is_last) done();
}

}  // namespace ft
