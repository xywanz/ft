// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "ctp/CtpMdApi.h"

#include <spdlog/spdlog.h>

#include <ThostFtdcUserApiDataType.h>
#include <ThostFtdcUserApiStruct.h>

#include "ctp/CtpCommon.h"
#include "MarketData.h"

namespace ft {

CtpMdApi::CtpMdApi(GeneralApi* general_api)
  : general_api_(general_api) {
}

CtpMdApi::~CtpMdApi() {
  is_error_ = true;
  if (ctp_api_)
    ctp_api_->Release();
}

bool CtpMdApi::login(const LoginParams& params) {
  if (params.broker_id().size() > sizeof(TThostFtdcBrokerIDType) ||
      params.broker_id().empty() ||
      params.investor_id().size() > sizeof(TThostFtdcUserIDType) ||
      params.investor_id().empty() ||
      params.passwd().size() > sizeof(TThostFtdcPasswordType) ||
      params.passwd().empty() ||
      params.md_server_addr().empty()) {
    spdlog::error("[CTP MD] Invalid login params");
    return false;
  }

  front_addr_ = params.md_server_addr();
  investor_id_ = params.investor_id();

  ctp_api_ = CThostFtdcMdApi::CreateFtdcMdApi();
  if (!ctp_api_) {
    spdlog::error("[CTP MD] Failed to create CTP MD API");
    return false;
  }

  ctp_api_->RegisterSpi(this);
  ctp_api_->RegisterFront(const_cast<char*>(params.md_server_addr().c_str()));
  ctp_api_->Init();

  for (;;) {
    if (is_error_)
      return false;

    if (is_connected_)
      break;
  }

  CThostFtdcReqUserLoginField login_req;
  memset(&login_req, 0, sizeof(login_req));
  strncpy(login_req.BrokerID, params.broker_id().c_str(), sizeof(login_req.BrokerID));
  strncpy(login_req.UserID, params.investor_id().c_str(), sizeof(login_req.UserID));
  strncpy(login_req.Password, params.passwd().c_str(), sizeof(login_req.Password));
  if (ctp_api_->ReqUserLogin(&login_req, next_req_id()) != 0) {
    spdlog::error("[CTP MD] Invalid user-login field");
    return false;
  }


  for (;;) {
    if (is_error_)
      return false;

    if (is_login_)
      break;
  }

  std::vector<char*> sub_list;
  std::string symbol;
  std::string exchange;
  for (const auto& ticker : params.subscribed_list()) {
    ticker_split(ticker, &symbol, &exchange);
    subscribed_list_.emplace_back(std::move(symbol));
  }

  for (const auto& p : subscribed_list_)
    sub_list.emplace_back(const_cast<char*>(p.c_str()));

  if (ctp_api_->SubscribeMarketData(sub_list.data(), sub_list.size()) != 0) {
    spdlog::error("[CTP MD] Failed to subscribe");
    return false;
  }

  return true;
}

void CtpMdApi::OnFrontConnected() {
  is_connected_ = true;
  spdlog::debug("[CTP MD] OnFrontConnected. Connected to the front {}", front_addr_);
}

void CtpMdApi::OnFrontDisconnected(int reason) {
  is_error_ = true;
  is_connected_ = false;
  spdlog::error("[CTP MD] OnFrontDisconnected. Disconnected from the front {}",
                front_addr_);
}

void CtpMdApi::OnHeartBeatWarning(int time_lapse) {
  spdlog::warn("[CTP MD] OnHeartBeatWarning. No packet received for a period of time");
}

void CtpMdApi::OnRspUserLogin(
                      CThostFtdcRspUserLoginField *rsp_user_login,
                      CThostFtdcRspInfoField *rsp_info,
                      int req_id,
                      bool is_last) {
  if (!is_last)
    return;

  if (is_error_rsp(rsp_info)) {
    spdlog::error("[CTP MD] OnRspUserLogin. Error ID: {}", rsp_info->ErrorID);
    is_error_ = true;
    return;
  }

  is_login_ = true;
  spdlog::debug("[CTP MD] OnRspUserLogin. Login as {}", investor_id_);
}

void CtpMdApi::OnRspUserLogout(
                      CThostFtdcUserLogoutField *uesr_logout,
                      CThostFtdcRspInfoField *rsp_info,
                      int req_id,
                      bool is_last) {
}

void CtpMdApi::OnRspError(
                      CThostFtdcRspInfoField *rsp_info,
                      int req_id,
                      bool is_last) {
}

void CtpMdApi::OnRspSubMarketData(
                      CThostFtdcSpecificInstrumentField *specific_instrument,
                      CThostFtdcRspInfoField *rsp_info,
                      int req_id,
                      bool is_last) {
  if (is_error_rsp(rsp_info) || !specific_instrument) {
    spdlog::error("[CTP MD] OnRspSubMarketData. Failed to subscribe. Error Msg: {}",
                  rsp_info->ErrorMsg);
    return;
  }

  auto* contract = ContractTable::get_by_symbol(specific_instrument->InstrumentID);
  if (!contract) {
    spdlog::error("[CTP MD] OnRspSubMarketData. ExchangeID not found in contract list. "
                  "Maybe you should update the contract list. Symbol: {}",
                  specific_instrument->InstrumentID);
    return;
  }
  symbol2exchange_.emplace(contract->symbol, contract->exchange);

  spdlog::debug("[CTP MD] OnRspSubMarketData. Successfully subscribe. Ticker: {}",
                contract->ticker);
}

void CtpMdApi::OnRspUnSubMarketData(
                      CThostFtdcSpecificInstrumentField *specific_instrument,
                      CThostFtdcRspInfoField *rsp_info,
                      int req_id,
                      bool is_last) {
}

void CtpMdApi::OnRspSubForQuoteRsp(
                      CThostFtdcSpecificInstrumentField *specific_instrument,
                      CThostFtdcRspInfoField *rsp_info,
                      int req_id,
                      bool is_last) {
}

void CtpMdApi::OnRspUnSubForQuoteRsp(
                      CThostFtdcSpecificInstrumentField *specific_instrument,
                      CThostFtdcRspInfoField *rsp_info,
                      int req_id,
                      bool is_last) {
}

void CtpMdApi::OnRtnDepthMarketData(
                      CThostFtdcDepthMarketDataField *md) {
  if (!md) {
    spdlog::error("[CTP MD] OnRtnDepthMarketData. Null pointer of market data");
    return;
  }

  auto iter = symbol2exchange_.find(md->InstrumentID);
  if (iter == symbol2exchange_.end()) {
    spdlog::warn("[CTP MD] OnRtnDepthMarketData. ExchangeID not found in contract list. "
                 "Maybe you should update the contract list. Symbol: {}",
                 md->InstrumentID);
    return;
  }

  MarketData tick;
  tick.symbol = md->InstrumentID;
  tick.exchange = iter->second;
  tick.ticker = to_ticker(tick.symbol, tick.exchange);

  struct tm _tm;
  strptime(md->UpdateTime, "%H:%M:%S", &_tm);
  tick.time_sec = _tm.tm_sec + _tm.tm_min * 60 + _tm.tm_hour * 3600;
  tick.time_ms = md->UpdateMillisec;
  tick.date = md->ActionDay;

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

  spdlog::debug("[CTP MD] OnRtnDepthMarketData. Ticker: {}, Time MS: {}, "
                "LastPrice: {:.2f}, Volume: {}, Turnover: {}, Open Interest: {}",
               tick.ticker, tick.time_ms, tick.last_price, tick.volume,
               tick.turnover, tick.open_interest);

  general_api_->on_tick(&tick);
}

void CtpMdApi::OnRtnForQuoteRsp(CThostFtdcForQuoteRspField *for_quote_rsp) {
}

}  // namespace ft
