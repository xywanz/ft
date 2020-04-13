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
      params.investor_id().size() > sizeof(TThostFtdcUserIDType) ||
      params.passwd().size() > sizeof(TThostFtdcPasswordType)) {
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
                      CThostFtdcDepthMarketDataField *depth_market_data) {
  if (!depth_market_data) {
    spdlog::error("[CTP MD] on_depth_md. Null pointer of market data");
  }

  MarketData data;
  data.symbol = depth_market_data->InstrumentID;
  data.exchange = depth_market_data->ExchangeID;
  // TODO(Kevin): 处理symbol、exchange、ticker的问题，因为行情是不带exchange的
  // 但是和交易相关的又是需要交由特定的交易所去处理，更新pnl的时候也是需要exchange
  // 这个信息，否则无法定位到指定的ticker
  data.ticker = to_ticker(data.symbol, "SHFE");

  data.time_ms = depth_market_data->UpdateMillisec;

  data.level = 5;
  data.ask[0] = depth_market_data->AskPrice1;
  data.ask[1] = depth_market_data->AskPrice2;
  data.ask[2] = depth_market_data->AskPrice3;
  data.ask[3] = depth_market_data->AskPrice4;
  data.ask[4] = depth_market_data->AskPrice5;
  data.bid[0] = depth_market_data->BidPrice1;
  data.bid[1] = depth_market_data->BidPrice2;
  data.bid[2] = depth_market_data->BidPrice3;
  data.bid[3] = depth_market_data->BidPrice4;
  data.bid[4] = depth_market_data->BidPrice5;
  data.ask_volume[0] = depth_market_data->AskVolume1;
  data.ask_volume[1] = depth_market_data->AskVolume2;
  data.ask_volume[2] = depth_market_data->AskVolume3;
  data.ask_volume[3] = depth_market_data->AskVolume4;
  data.ask_volume[4] = depth_market_data->AskVolume5;
  data.bid_volume[0] = depth_market_data->BidVolume1;
  data.bid_volume[1] = depth_market_data->BidVolume2;
  data.bid_volume[2] = depth_market_data->BidVolume3;
  data.bid_volume[3] = depth_market_data->BidVolume4;
  data.bid_volume[4] = depth_market_data->BidVolume5;

  data.last_price = depth_market_data->LastPrice;

  data.volume = depth_market_data->Volume;
  data.turnover = depth_market_data->Turnover;

  data.open_interest = depth_market_data->OpenInterest;

  spdlog::debug("[CTP MD] on_depth_md. Ticker: {}, Time MS: {}, LastPrice: {:.2f}, "
                "Volume: {}, Turnover: {}, Open Interest: {}",
               data.ticker, data.time_ms, data.last_price, data.volume,
               data.turnover, data.open_interest);

  trader_->on_market_data(&data);
}

void CtpMdReceiver::on_for_quote_rsp(CThostFtdcForQuoteRspField *for_quote_rsp) {
}

}  // namespace ft
