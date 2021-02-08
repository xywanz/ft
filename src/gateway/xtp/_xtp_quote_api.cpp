// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "gateway/xtp/_xtp_quote_api.h"

#include <spdlog/spdlog.h>

#include <vector>

#include "utils/contract_table.h"
#include "utils/misc.h"

namespace ft {

XtpQuoteApi::XtpQuoteApi(BaseOrderManagementSystem* oms) : oms_(oms) {}

XtpQuoteApi::~XtpQuoteApi() {
  error();
  Logout();
}

bool XtpQuoteApi::Login(const Config& config) {
  if (is_logon_) {
    spdlog::error("[XtpQuoteApi::Login] Don't Login twice");
    return false;
  }

  uint32_t seed = time(nullptr);
  uint8_t client_id = rand_r(&seed) & 0xff;
  quote_api_.reset(XTP::API::QuoteApi::CreateQuoteApi(client_id, "."));
  if (!quote_api_) {
    spdlog::error("[XtpQuoteApi::Login] Failed to CreateQuoteApi");
    return false;
  }

  char protocol[32]{};
  char ip[32]{};
  int port = 0;

  try {
    int ret = sscanf(config.quote_server_address.c_str(), "%[^:]://%[^:]:%d", protocol, ip, &port);
    if (ret != 3) {
      spdlog::error("FAILED to parse server address: {}", config.quote_server_address);
      return false;
    }
  } catch (...) {
    spdlog::error("FAILED to parse server address: {}", config.quote_server_address);
    return false;
  }

  XTP_PROTOCOL_TYPE sock_type = XTP_PROTOCOL_TCP;
  if (strcmp(protocol, "udp") == 0) sock_type = XTP_PROTOCOL_UDP;

  quote_api_->RegisterSpi(this);
  if (quote_api_->Login(ip, port, config.investor_id.c_str(), config.password.c_str(), sock_type) !=
      0) {
    spdlog::error("[XtpQuoteApi::Login] Failed to Login: {}",
                  quote_api_->GetApiLastError()->error_msg);
    return false;
  }

  spdlog::debug("[XtpQuoteApi::Login] Success");
  is_logon_ = true;
  return true;
}

void XtpQuoteApi::Logout() {
  if (is_logon_) {
    quote_api_->Logout();
    is_logon_ = false;
  }
}

bool XtpQuoteApi::Subscribe(const std::vector<std::string>& sub_list) {
  subscribed_list_ = sub_list;
  std::vector<char*> sub_list_sh;
  std::vector<char*> sub_list_sz;
  for (auto& ticker : subscribed_list_) {
    auto contract = ContractTable::get_by_ticker(ticker);
    assert(contract);
    if (contract->exchange == exchange::kSSE)
      sub_list_sh.emplace_back(const_cast<char*>(ticker.c_str()));
    else if (contract->exchange == exchange::kSZE)
      sub_list_sz.emplace_back(const_cast<char*>(ticker.c_str()));
  }

  if (sub_list_sh.size() > 0) {
    if (quote_api_->SubscribeMarketData(sub_list_sh.data(), sub_list_sh.size(), XTP_EXCHANGE_SH) !=
        0) {
      spdlog::error("[XtpQuoteApi::Login] 无法订阅行情");
      return false;
    }
  }

  if (sub_list_sz.size() > 0) {
    if (quote_api_->SubscribeMarketData(sub_list_sz.data(), sub_list_sz.size(), XTP_EXCHANGE_SZ) !=
        0) {
      spdlog::error("[XtpQuoteApi::Login] 无法订阅行情");
      return false;
    }
  }

  return true;
}

bool XtpQuoteApi::QueryContractList(std::vector<Contract>* result) {
  if (!is_logon_) {
    spdlog::error("[XtpQuoteApi::QueryContractList] 未登录到quote服务器");
    return false;
  }

  contract_results_ = result;
  if (quote_api_->QueryAllTickers(XTP_EXCHANGE_SH) != 0) {
    spdlog::error("[XtpQuoteApi::QueryContract] Failed to query SH stocks");
    return false;
  }
  if (!wait_sync()) {
    spdlog::error("[XtpQuoteApi::QueryContract] Failed to query SH stocks");
    return false;
  }

  if (quote_api_->QueryAllTickers(XTP_EXCHANGE_SZ) != 0) {
    spdlog::error("[XtpQuoteApi::QueryContract] Failed to query SZ stocks");
    return false;
  }
  if (!wait_sync()) {
    spdlog::error("[XtpQuoteApi::QueryContract] Failed to query SZ stocks");
    return false;
  }

  return true;
}

void XtpQuoteApi::OnQueryAllTickers(XTPQSI* ticker_info, XTPRI* error_info, bool is_last) {
  if (is_error_rsp(error_info)) {
    spdlog::error("[XtpQuoteApi::OnQueryAllTickers] {}", error_info->error_msg);
    error();
    return;
  }

  if (ticker_info && (ticker_info->ticker_type == XTP_TICKER_TYPE_STOCK ||
                      ticker_info->ticker_type == XTP_TICKER_TYPE_FUND)) {
    spdlog::debug("[XtpQuoteApi::OnQueryAllTickers] {}, {}", ticker_info->ticker,
                  ticker_info->ticker_name);
    Contract contract{};
    contract.ticker = ticker_info->ticker;
    contract.exchange = ft_exchange_type(ticker_info->exchange_id);
    contract.name = ticker_info->ticker_name;
    contract.price_tick = ticker_info->price_tick;
    contract.long_margin_rate = 1.0;
    contract.short_margin_rate = 1.0;
    if (ticker_info->ticker_type == XTP_TICKER_TYPE_STOCK)
      contract.product_type = ProductType::kStock;
    else if (ticker_info->ticker_type == XTP_TICKER_TYPE_FUND)
      contract.product_type = ProductType::kFund;
    contract.size = 1;

    if (contract_results_) contract_results_->emplace_back(contract);
  }

  if (is_last) done();
}

void XtpQuoteApi::OnDepthMarketData(XTPMD* market_data, int64_t bid1_qty[], int32_t bid1_count,
                                    int32_t max_bid1_count, int64_t ask1_qty[], int32_t ask1_count,
                                    int32_t max_ask1_count) {
  if (!market_data) {
    spdlog::warn("[XtpQuoteApi::OnDepthMarketData] nullptr");
    return;
  }

  auto contract = ContractTable::get_by_ticker(market_data->ticker);
  if (!contract) {
    spdlog::trace("[XtpQuoteApi::OnDepthMarketData] {} not found int ContractTable",
                  market_data->ticker);
    return;
  }

  TickData tick{};
  tick.source = MarketDataSource::kXTP;
  tick.ticker_id = contract->ticker_id;

  uint64_t sec = (market_data->data_time / 1000UL) % 100;
  uint64_t min = (market_data->data_time / 100000UL) % 100;
  uint64_t hour = (market_data->data_time / 10000000UL) % 100;
  uint64_t msec = market_data->data_time % 1000;
  tick.time_us = (sec + 60UL * min + 3600UL * hour) * 1000000UL + msec * 1000UL;
  time_t local_time;
  strftime(tick.date, sizeof(tick.date), "%Y%m%d", localtime(&local_time));

  tick.volume = market_data->qty;
  tick.turnover = market_data->turnover;
  tick.open_interest = market_data->total_long_positon;
  tick.last_price = market_data->last_price;
  tick.open_price = market_data->open_price;
  tick.highest_price = market_data->high_price;
  tick.lowest_price = market_data->low_price;
  tick.pre_close_price = market_data->pre_close_price;
  tick.upper_limit_price = market_data->upper_limit_price;
  tick.lower_limit_price = market_data->lower_limit_price;
  tick.etf.iopv = market_data->stk.iopv;  // 只对于ETF有效

  tick.level = 10;
  tick.ask[0] = market_data->ask[0];
  tick.ask[1] = market_data->ask[1];
  tick.ask[2] = market_data->ask[2];
  tick.ask[3] = market_data->ask[3];
  tick.ask[4] = market_data->ask[4];
  tick.ask[0] = market_data->ask[5];
  tick.ask[1] = market_data->ask[6];
  tick.ask[2] = market_data->ask[7];
  tick.ask[3] = market_data->ask[8];
  tick.ask[4] = market_data->ask[9];
  tick.ask_volume[0] = market_data->ask_qty[0];
  tick.ask_volume[1] = market_data->ask_qty[1];
  tick.ask_volume[2] = market_data->ask_qty[2];
  tick.ask_volume[3] = market_data->ask_qty[3];
  tick.ask_volume[4] = market_data->ask_qty[4];
  tick.ask_volume[0] = market_data->ask_qty[5];
  tick.ask_volume[1] = market_data->ask_qty[6];
  tick.ask_volume[2] = market_data->ask_qty[7];
  tick.ask_volume[3] = market_data->ask_qty[8];
  tick.ask_volume[4] = market_data->ask_qty[9];
  tick.bid[0] = market_data->bid[0];
  tick.bid[1] = market_data->bid[1];
  tick.bid[2] = market_data->bid[2];
  tick.bid[3] = market_data->bid[3];
  tick.bid[4] = market_data->bid[4];
  tick.bid[0] = market_data->bid[5];
  tick.bid[1] = market_data->bid[6];
  tick.bid[2] = market_data->bid[7];
  tick.bid[3] = market_data->bid[8];
  tick.bid[4] = market_data->bid[9];
  tick.bid_volume[0] = market_data->bid_qty[0];
  tick.bid_volume[1] = market_data->bid_qty[1];
  tick.bid_volume[2] = market_data->bid_qty[2];
  tick.bid_volume[3] = market_data->bid_qty[3];
  tick.bid_volume[4] = market_data->bid_qty[4];
  tick.bid_volume[0] = market_data->bid_qty[5];
  tick.bid_volume[1] = market_data->bid_qty[6];
  tick.bid_volume[2] = market_data->bid_qty[7];
  tick.bid_volume[3] = market_data->bid_qty[8];
  tick.bid_volume[4] = market_data->bid_qty[9];

  spdlog::trace(
      "[XtpQuoteApi::OnRtnDepthMarketData] {}, TimeUS: {}, LastPrice:{:.2f}, "
      "Volume:{}, Turnover:{}, Open Interest:{}",
      market_data->ticker, tick.time_us, tick.last_price, tick.volume, tick.turnover,
      tick.open_interest);

  oms_->OnTick(&tick);
}

}  // namespace ft
