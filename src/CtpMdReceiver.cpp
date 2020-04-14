// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "ctp/CtpMdReceiver.h"

#include <spdlog/spdlog.h>

#include <ThostFtdcUserApiDataType.h>
#include <ThostFtdcUserApiStruct.h>

#include "ctp/CtpCommon.h"
#include "ctp/CtpMdSpi.h"
#include "MarketData.h"

namespace ft {

CtpMdReceiver::CtpMdReceiver() {
}

bool CtpMdReceiver::login(const LoginParams& params) {
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

  api_ = CThostFtdcMdApi::CreateFtdcMdApi();
  if (!api_) {
    spdlog::error("[CTP MD] Failed to create CTP MD API");
    return false;
  }

  spi_ = new CtpMdSpi(this);
  api_->RegisterSpi(spi_);
  api_->RegisterFront(const_cast<char*>(params.md_server_addr().c_str()));
  api_->Init();
  while (!is_connected_)
    continue;

  int req_id;
  AsyncStatus status;

  CThostFtdcReqUserLoginField login_req;
  memset(&login_req, 0, sizeof(login_req));
  strncpy(login_req.BrokerID, params.broker_id().c_str(), sizeof(login_req.BrokerID));
  strncpy(login_req.UserID, params.investor_id().c_str(), sizeof(login_req.UserID));
  strncpy(login_req.Password, params.passwd().c_str(), sizeof(login_req.Password));
  req_id = next_req_id();
  status = req_async_status(req_id);
  if (api_->ReqUserLogin(&login_req, req_id) != 0) {
    spdlog::error("[CTP MD] Invalid user-login field");
    rsp_async_status(req_id, false);
  }

  if (!status.wait()) {
    spdlog::error("[CTP MD] Failed to login");
    return false;
  }
  is_login_ = true;

  const auto& sub_list = params.subscribed_list();
  std::vector<char*> symbol_list;
  std::string symbol;
  std::string exchange;
  for (const auto& ticker : sub_list) {
    ticker_split(ticker, &symbol, &exchange);
    subscribed_list_.emplace_back(std::move(symbol));
    symbol_list.emplace_back(const_cast<char*>(subscribed_list_.back().c_str()));
  }

  if (api_->SubscribeMarketData(symbol_list.data(), symbol_list.size()) != 0) {
    spdlog::error("[CTP MD] Failed to subscribe");
    return false;
  }

  return true;
}

void CtpMdReceiver::on_connected() {
  is_connected_ = true;
  spdlog::debug("[CTP MD] on_connected. Connected to the front {}", front_addr_);
}

void CtpMdReceiver::on_disconnected(int reason) {
  is_connected_ = false;
  spdlog::error("[CTP MD] on_disconnected. Disconnected from the front {}", front_addr_);
}

void CtpMdReceiver::on_heart_beat_warning(int time_lapse) {
  spdlog::warn("[CTP MD] on_heart_beat_warning. No packet received for a period of time");
}

void CtpMdReceiver::on_login(
                      CThostFtdcRspUserLoginField *rsp_user_login,
                      CThostFtdcRspInfoField *rsp_info,
                      int req_id,
                      bool is_last) {
  if (!is_last)
    return;

  if (is_error_rsp(rsp_info)) {
    spdlog::error("[CTP MD] on_login. Error ID: {}", rsp_info->ErrorID);
    rsp_async_status(req_id, false);
    return;
  }

  rsp_async_status(req_id, true);
  spdlog::debug("[CTP MD] on_login. Login as {}", investor_id_);
}

void CtpMdReceiver::on_logout(
                      CThostFtdcUserLogoutField *uesr_logout,
                      CThostFtdcRspInfoField *rsp_info,
                      int req_id,
                      bool is_last) {
}

void CtpMdReceiver::on_error(
                      CThostFtdcRspInfoField *rsp_info,
                      int req_id,
                      bool is_last) {
}

void CtpMdReceiver::on_sub_md(
                      CThostFtdcSpecificInstrumentField *specific_instrument,
                      CThostFtdcRspInfoField *rsp_info,
                      int req_id,
                      bool is_last) {
  if (is_error_rsp(rsp_info) || !specific_instrument) {
    spdlog::error("[CTP MD] on_sub_md. Failed to subscribe. Error Msg: {}",
                  rsp_info->ErrorMsg);
    return;
  }

  spdlog::debug("[CTP MD] on_sub_md. Successfully subscribe. Instrument: {}",
               specific_instrument->InstrumentID);
}

void CtpMdReceiver::on_unsub_md(
                      CThostFtdcSpecificInstrumentField *specific_instrument,
                      CThostFtdcRspInfoField *rsp_info,
                      int req_id,
                      bool is_last) {
}

void CtpMdReceiver::on_sub_for_quote_rsp(
                      CThostFtdcSpecificInstrumentField *specific_instrument,
                      CThostFtdcRspInfoField *rsp_info,
                      int req_id,
                      bool is_last) {
}

void CtpMdReceiver::on_unsub_for_quote_rsp(
                      CThostFtdcSpecificInstrumentField *specific_instrument,
                      CThostFtdcRspInfoField *rsp_info,
                      int req_id,
                      bool is_last) {
}

void CtpMdReceiver::on_depth_md(
                      CThostFtdcDepthMarketDataField *md) {
  if (!md) {
    spdlog::error("[CTP MD] on_depth_md. Null pointer of market data");
    return;
  }

  MarketData tick;
  tick.symbol = md->InstrumentID;
  // TODO(Kevin): 处理symbol、exchange、ticker的问题，因为行情是不带exchange的
  // 但是和交易相关的又是需要交由特定的交易所去处理，更新pnl的时候也是需要exchange
  // 这个信息，否则无法定位到指定的ticker
  // tick.exchange = md->ExchangeID;
  tick.exchange = "SHFE";
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

  spdlog::debug("[CTP MD] on_depth_md. Ticker: {}, Time MS: {}, LastPrice: {:.2f}, "
                "Volume: {}, Turnover: {}, Open Interest: {}",
               tick.ticker, tick.time_ms, tick.last_price, tick.volume,
               tick.turnover, tick.open_interest);

  trader_->on_market_data(&tick);
}

void CtpMdReceiver::on_for_quote_rsp(CThostFtdcForQuoteRspField *for_quote_rsp) {
}

}  // namespace ft
