// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "Api/Ctp/CtpMdApi.h"

#include <ThostFtdcUserApiDataType.h>
#include <ThostFtdcUserApiStruct.h>
#include <spdlog/spdlog.h>

#include "Api/Ctp/CtpCommon.h"
#include "Base/DataStruct.h"
#include "ContractTable.h"

namespace ft {

CtpMdApi::CtpMdApi(Gateway *gateway) : gateway_(gateway) {}

CtpMdApi::~CtpMdApi() {
  is_error_ = true;
  logout();
}

bool CtpMdApi::login(const LoginParams &params) {
  if (params.broker_id().size() > sizeof(TThostFtdcBrokerIDType) ||
      params.broker_id().empty() ||
      params.investor_id().size() > sizeof(TThostFtdcUserIDType) ||
      params.investor_id().empty() ||
      params.passwd().size() > sizeof(TThostFtdcPasswordType) ||
      params.passwd().empty() || params.md_server_addr().empty()) {
    spdlog::error("[CtpMdApi::login] Failed. Invalid login params");
    return false;
  }

  front_addr_ = params.md_server_addr();
  broker_id_ = params.broker_id();
  investor_id_ = params.investor_id();

  ctp_api_.reset(CThostFtdcMdApi::CreateFtdcMdApi());
  if (!ctp_api_) {
    spdlog::error("[CtpMdApi::login] Failed to create CTP MD API");
    return false;
  }

  ctp_api_->RegisterSpi(this);
  ctp_api_->RegisterFront(const_cast<char *>(params.md_server_addr().c_str()));
  ctp_api_->Init();

  for (;;) {
    if (is_error_) return false;

    if (is_connected_) break;
  }

  CThostFtdcReqUserLoginField login_req;
  memset(&login_req, 0, sizeof(login_req));
  strncpy(login_req.BrokerID, params.broker_id().c_str(),
          sizeof(login_req.BrokerID));
  strncpy(login_req.UserID, params.investor_id().c_str(),
          sizeof(login_req.UserID));
  strncpy(login_req.Password, params.passwd().c_str(),
          sizeof(login_req.Password));
  if (ctp_api_->ReqUserLogin(&login_req, next_req_id()) != 0) {
    spdlog::error("[CtpMdApi::login] Invalid user-login field");
    return false;
  }

  for (;;) {
    if (is_error_) return false;

    if (is_login_) break;
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

  if (ctp_api_->SubscribeMarketData(sub_list.data(), sub_list.size()) != 0) {
    spdlog::error("[CtpMdApi::login] Failed to subscribe");
    return false;
  }

  return true;
}

bool CtpMdApi::logout() {
  if (is_login_) {
    CThostFtdcUserLogoutField req;
    strncpy(req.BrokerID, broker_id_.c_str(), sizeof(req.BrokerID));
    strncpy(req.UserID, investor_id_.c_str(), sizeof(req.UserID));
    if (ctp_api_->ReqUserLogout(&req, next_req_id()) != 0) return false;

    while (is_login_) continue;
  }

  return true;
}

void CtpMdApi::OnFrontConnected() {
  is_connected_ = true;
  spdlog::debug("[CtpMdApi::OnFrontConnected] Connected to {}", front_addr_);
}

void CtpMdApi::OnFrontDisconnected(int reason) {
  is_error_ = true;
  is_connected_ = false;
  spdlog::error("[CtpMdApi::OnFrontDisconnected] Disconnected from {}",
                front_addr_);
}

void CtpMdApi::OnHeartBeatWarning(int time_lapse) {
  spdlog::warn(
      "[CtpMdApi::OnHeartBeatWarning] Warn. No packet received for a period of "
      "time");
}

void CtpMdApi::OnRspUserLogin(CThostFtdcRspUserLoginField *login_rsp,
                              CThostFtdcRspInfoField *rsp_info, int req_id,
                              bool is_last) {
  if (!is_last) return;

  if (is_error_rsp(rsp_info)) {
    spdlog::error("[CtpMdApi::OnRspUserLogin] Failed. ErrorMsg: {}",
                  gb2312_to_utf8(rsp_info->ErrorMsg));
    is_error_ = true;
    return;
  }

  spdlog::debug("[CtpMdApi::OnRspUserLogin] Success. Login as {}",
                investor_id_);
  is_login_ = true;
}

void CtpMdApi::OnRspUserLogout(CThostFtdcUserLogoutField *logout_rsp,
                               CThostFtdcRspInfoField *rsp_info, int req_id,
                               bool is_last) {
  spdlog::debug(
      "[CtpMdApi::OnRspUserLogout] Success. Broker ID: {}, Investor ID: {}",
      logout_rsp->BrokerID, logout_rsp->UserID);
  is_login_ = false;
}

void CtpMdApi::OnRspError(CThostFtdcRspInfoField *rsp_info, int req_id,
                          bool is_last) {
  spdlog::debug("[CtpMdApi::OnRspError] ErrorMsg: {}",
                gb2312_to_utf8(rsp_info->ErrorMsg));
  is_login_ = false;
}

void CtpMdApi::OnRspSubMarketData(CThostFtdcSpecificInstrumentField *instrument,
                                  CThostFtdcRspInfoField *rsp_info, int req_id,
                                  bool is_last) {
  if (is_error_rsp(rsp_info) || !instrument) {
    spdlog::error("[CtpMdApi::OnRspSubMarketData] Failed. Error Msg: {}",
                  gb2312_to_utf8(rsp_info->ErrorMsg));
    return;
  }

  auto *contract = ContractTable::get_by_symbol(instrument->InstrumentID);
  if (!contract) {
    spdlog::error(
        "[CtpMdApi::OnRspSubMarketData] Failed. ExchangeID not found in "
        "contract list. "
        "Maybe you should update the contract list. Symbol: {}",
        instrument->InstrumentID);
    return;
  }
  symbol2exchange_.emplace(contract->symbol, contract->exchange);

  spdlog::debug("[CtpMdApi::OnRspSubMarketData] Success. Ticker: {}",
                contract->ticker);
}

void CtpMdApi::OnRspUnSubMarketData(
    CThostFtdcSpecificInstrumentField *instrument,
    CThostFtdcRspInfoField *rsp_info, int req_id, bool is_last) {}

void CtpMdApi::OnRspSubForQuoteRsp(
    CThostFtdcSpecificInstrumentField *instrument,
    CThostFtdcRspInfoField *rsp_info, int req_id, bool is_last) {}

void CtpMdApi::OnRspUnSubForQuoteRsp(
    CThostFtdcSpecificInstrumentField *instrument,
    CThostFtdcRspInfoField *rsp_info, int req_id, bool is_last) {}

void CtpMdApi::OnRtnDepthMarketData(CThostFtdcDepthMarketDataField *md) {
  if (!md) {
    spdlog::error("[CtpMdApi::OnRtnDepthMarketData] Failed. md is nullptr");
    return;
  }

  auto iter = symbol2exchange_.find(md->InstrumentID);
  if (iter == symbol2exchange_.end()) {
    spdlog::warn(
        "[CtpMdApi::OnRtnDepthMarketData] Failed. ExchangeID not found in "
        "contract list. "
        "Maybe you should update the contract list. Symbol: {}",
        md->InstrumentID);
    return;
  }

  TickData tick;
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

  spdlog::debug(
      "[CtpMdApi::OnRtnDepthMarketData] Ticker: {}, Time MS: {}, "
      "LastPrice: {:.2f}, Volume: {}, Turnover: {}, Open Interest: {}",
      tick.ticker, tick.time_ms, tick.last_price, tick.volume, tick.turnover,
      tick.open_interest);

  gateway_->on_tick(&tick);
}

void CtpMdApi::OnRtnForQuoteRsp(CThostFtdcForQuoteRspField *for_quote_rsp) {}

}  // namespace ft
