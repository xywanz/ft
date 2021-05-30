// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "trader/gateway/ctp/ctp_quote_api.h"

#include <utility>

#include "ft/base/log.h"
#include "trader/gateway/ctp/ctp_gateway.h"

namespace ft {

CtpQuoteApi::CtpQuoteApi(CtpGateway *gateway) : gateway_(gateway) {}

CtpQuoteApi::~CtpQuoteApi() {
  if (quote_api_) {
    Logout();
    quote_api_->Join();
  }
}

bool CtpQuoteApi::Login(const GatewayConfig &config) {
  quote_api_.reset(CThostFtdcMdApi::CreateFtdcMdApi());
  if (!quote_api_) {
    LOG_ERROR("[CtpQuoteApi::Login] Failed to create CTP MD API");
    return false;
  }

  server_addr_ = config.quote_server_address;
  broker_id_ = config.broker_id;
  investor_id_ = config.investor_id;
  passwd_ = config.password;

  quote_api_->RegisterSpi(this);
  quote_api_->RegisterFront(const_cast<char *>(server_addr_.c_str()));
  quote_api_->Init();

  while (status_.load(std::memory_order::memory_order_relaxed) == 0) {
    continue;
  }
  return status_.load(std::memory_order::memory_order_acquire) == 1;
}

void CtpQuoteApi::Logout() {
  CThostFtdcUserLogoutField req{};
  strncpy(req.BrokerID, broker_id_.c_str(), sizeof(req.BrokerID));
  strncpy(req.UserID, investor_id_.c_str(), sizeof(req.UserID));
  quote_api_->ReqUserLogout(&req, next_req_id());
}

bool CtpQuoteApi::Subscribe(const std::vector<std::string> &_sub_list) {
  std::vector<char *> sub_list;
  sub_list_ = _sub_list;

  for (const auto &p : sub_list_) sub_list.emplace_back(const_cast<char *>(p.c_str()));

  if (sub_list.size() > 0) {
    if (quote_api_->SubscribeMarketData(sub_list.data(), sub_list.size()) != 0) {
      LOG_ERROR("[CtpQuoteApi::Subscribe] Failed to Subscribe");
      return false;
    }
  }
  return true;
}

void CtpQuoteApi::OnFrontConnected() {
  LOG_DEBUG("[CtpQuoteApi::OnFrontConnectedMD] Connected");

  CThostFtdcReqUserLoginField login_req{};
  strncpy(login_req.BrokerID, broker_id_.c_str(), sizeof(login_req.BrokerID));
  strncpy(login_req.UserID, investor_id_.c_str(), sizeof(login_req.UserID));
  strncpy(login_req.Password, passwd_.c_str(), sizeof(login_req.Password));
  if (quote_api_->ReqUserLogin(&login_req, next_req_id()) != 0) {
    LOG_ERROR("[CtpQuoteApi::Login] Invalid user-Login field");
    status_.store(-1, std::memory_order::memory_order_release);
  }
}

void CtpQuoteApi::OnFrontDisconnected(int reason) {
  LOG_ERROR("[CtpQuoteApi::OnFrontDisconnectedMD] Disconnected");
  status_.store(0, std::memory_order::memory_order_relaxed);
}

void CtpQuoteApi::OnHeartBeatWarning(int time_lapse) {
  LOG_WARN(
      "[CtpQuoteApi::OnHeartBeatWarningMD] Warn. No packet received for a "
      "period of time");
}

void CtpQuoteApi::OnRspUserLogin(CThostFtdcRspUserLoginField *login_rsp,
                                 CThostFtdcRspInfoField *rsp_info, int req_id, bool is_last) {
  if (!is_last) return;

  if (is_error_rsp(rsp_info)) {
    LOG_ERROR("[CtpQuoteApi::OnRspUserLogin] Failed. ErrorMsg: {}",
              gb2312_to_utf8(rsp_info->ErrorMsg));
    status_.store(-1, std::memory_order::memory_order_release);
    return;
  }

  LOG_DEBUG("[CtpQuoteApi::OnRspUserLogin] Success. Login as {}", investor_id_);
  status_.store(1, std::memory_order::memory_order_release);
}

void CtpQuoteApi::OnRspUserLogout(CThostFtdcUserLogoutField *logout_rsp,
                                  CThostFtdcRspInfoField *rsp_info, int req_id, bool is_last) {
  LOG_DEBUG("[CtpQuoteApi::OnRspUserLogout] Success. Broker ID: {}, Investor ID: {}",
            logout_rsp->BrokerID, logout_rsp->UserID);
  status_.store(0, std::memory_order::memory_order_relaxed);
}

void CtpQuoteApi::OnRspError(CThostFtdcRspInfoField *rsp_info, int req_id, bool is_last) {
  LOG_DEBUG("[CtpQuoteApi::OnRspError] ErrorMsg: {}", gb2312_to_utf8(rsp_info->ErrorMsg));
  status_.store(0, std::memory_order::memory_order_relaxed);
}

void CtpQuoteApi::OnRspSubMarketData(CThostFtdcSpecificInstrumentField *instrument,
                                     CThostFtdcRspInfoField *rsp_info, int req_id, bool is_last) {
  if (is_error_rsp(rsp_info) || !instrument) {
    LOG_ERROR("[CtpQuoteApi::OnRspSubMarketData] Failed. Error Msg: {}",
              gb2312_to_utf8(rsp_info->ErrorMsg));
    return;
  }

  auto contract = ContractTable::get_by_ticker(instrument->InstrumentID);
  if (!contract) {
    LOG_ERROR(
        "[CtpQuoteApi::OnRspSubMarketData] ExchangeID not found in contract "
        "list. Maybe you should update the contract list. Ticker: {}",
        instrument->InstrumentID);
    return;
  }

  LOG_DEBUG("[CtpQuoteApi::OnRspSubMarketData] Success. Ticker: {}", contract->ticker);
}

void CtpQuoteApi::OnRspUnSubMarketData(CThostFtdcSpecificInstrumentField *instrument,
                                       CThostFtdcRspInfoField *rsp_info, int req_id, bool is_last) {
}

void CtpQuoteApi::OnRspSubForQuoteRsp(CThostFtdcSpecificInstrumentField *instrument,
                                      CThostFtdcRspInfoField *rsp_info, int req_id, bool is_last) {}

void CtpQuoteApi::OnRspUnSubForQuoteRsp(CThostFtdcSpecificInstrumentField *instrument,
                                        CThostFtdcRspInfoField *rsp_info, int req_id,
                                        bool is_last) {}

void CtpQuoteApi::OnRtnDepthMarketData(CThostFtdcDepthMarketDataField *md) {
  if (!md) {
    LOG_ERROR("[CtpQuoteApi::OnRtnDepthMarketData] Failed. md is nullptr");
    return;
  }

  timespec ts;
  clock_gettime(CLOCK_REALTIME, &ts);

  auto contract = ContractTable::get_by_ticker(md->InstrumentID);
  if (!contract) {
    LOG_WARN(
        "[CtpQuoteApi::OnRtnDepthMarketData] Failed. ExchangeID not found in "
        "contract list. "
        "Maybe you should update the contract list. Symbol: {}",
        md->InstrumentID);
    return;
  }

  dt_converter_.UpdateDate(md->ActionDay);

  TickData tick{};
  tick.local_timestamp_us = ts.tv_sec * 1000000UL + ts.tv_nsec / 1000UL;
  tick.source = MarketDataSource::kCTP;
  tick.ticker_id = contract->ticker_id;
  tick.exchange_timestamp_us = dt_converter_.GetExchTimeStamp(md->UpdateTime, md->UpdateMillisec);
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

  LOG_TRACE(
      "[CtpQuoteApi::OnRtnDepthMarketData] {}, ExchangeDatetime:{}, TimeUS:{}, "
      "LastPrice:{:.2f}, Volume:{}, Turnover:{}, OpenInterest:{}",
      contract->ticker, md->ActionDay, tick.exchange_timestamp_us, tick.last_price, tick.volume,
      tick.turnover, tick.open_interest);

  gateway_->OnTick(tick);
}

void CtpQuoteApi::OnRtnForQuoteRsp(CThostFtdcForQuoteRspField *for_quote_rsp) {}

}  // namespace ft
