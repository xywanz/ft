// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "Gateway/Xtp/XtpMdApi.h"

#include <spdlog/spdlog.h>

#include <vector>

#include "Core/ContractTable.h"

namespace ft {

XtpMdApi::XtpMdApi(TradingEngineInterface* engine) : engine_(engine) {}

XtpMdApi::~XtpMdApi() {
  error();
  logout();
}

bool XtpMdApi::login(const Config& config) {
  if (is_logon_) {
    spdlog::error("[XtpMdApi::login] Don't login twice");
    return false;
  }

  uint32_t seed = time(nullptr);
  uint8_t client_id = rand_r(&seed) & 0xff;
  quote_api_.reset(XTP::API::QuoteApi::CreateQuoteApi(client_id, "."));
  if (!quote_api_) {
    spdlog::error("[XtpMdApi::login] Failed to CreateQuoteApi");
    return false;
  }

  char protocol[32]{};
  char ip[32]{};
  int port = 0;

  try {
    int ret = sscanf(config.quote_server_address.c_str(), "%[^:]://%[^:]:%d",
                     protocol, ip, &port);
    assert(ret == 3);
  } catch (...) {
    assert(false);
  }

  XTP_PROTOCOL_TYPE sock_type = XTP_PROTOCOL_TCP;
  if (strcmp(protocol, "udp") == 0) sock_type = XTP_PROTOCOL_UDP;

  quote_api_->RegisterSpi(this);
  if (quote_api_->Login(ip, port, config.investor_id.c_str(),
                        config.password.c_str(), sock_type) != 0) {
    spdlog::error("[XtpMdApi::login] Failed to login: {}",
                  quote_api_->GetApiLastError()->error_msg);
    return false;
  }

  spdlog::debug("[XtpMdApi::login] Success");
  is_logon_ = true;

  subscribed_list_ = config.subscription_list;
  std::vector<char*> sub_list_sh;
  std::vector<char*> sub_list_sz;
  for (auto& ticker : subscribed_list_) {
    auto contract = ContractTable::get_by_ticker(ticker);
    assert(contract);
    if (contract->exchange == SSE)
      sub_list_sh.emplace_back(const_cast<char*>(ticker.c_str()));
    else if (contract->exchange == SZE)
      sub_list_sz.emplace_back(const_cast<char*>(ticker.c_str()));
  }

  if (sub_list_sh.size() > 0) {
    if (quote_api_->SubscribeMarketData(sub_list_sh.data(), sub_list_sh.size(),
                                        XTP_EXCHANGE_SH) != 0) {
      spdlog::error("[XtpMdApi::login] 无法订阅行情");
      return false;
    }
  }

  if (sub_list_sz.size() > 0) {
    if (quote_api_->SubscribeMarketData(sub_list_sz.data(), sub_list_sz.size(),
                                        XTP_EXCHANGE_SZ) != 0) {
      spdlog::error("[XtpMdApi::login] 无法订阅行情");
      return false;
    }
  }

  return true;
}

void XtpMdApi::logout() {
  if (is_logon_) {
    quote_api_->Logout();
    is_logon_ = false;
  }
}

bool XtpMdApi::query_contract(const std::string& ticker,
                              const std::string& exchange) {
  spdlog::error("[XtpMdApi::query_contract] XTP不支持查询单个合约信息");
  return false;
}

bool XtpMdApi::query_contracts() {
  if (!is_logon_) {
    spdlog::error("[XtpMdApi::query_contracts] 未登录到quote服务器");
    return false;
  }

  std::unique_lock<std::mutex> lock(query_mutex_);
  if (quote_api_->QueryAllTickers(XTP_EXCHANGE_SH) != 0) {
    spdlog::error("[XtpMdApi::query_contract] Failed to query SH stocks");
    return false;
  }
  if (!wait_sync()) {
    spdlog::error("[XtpMdApi::query_contract] Failed to query SH stocks");
    return false;
  }

  if (quote_api_->QueryAllTickers(XTP_EXCHANGE_SZ) != 0) {
    spdlog::error("[XtpMdApi::query_contract] Failed to query SZ stocks");
    return false;
  }
  if (!wait_sync()) {
    spdlog::error("[XtpMdApi::query_contract] Failed to query SZ stocks");
    return false;
  }

  return true;
}

void XtpMdApi::OnQueryAllTickers(XTPQSI* ticker_info, XTPRI* error_info,
                                 bool is_last) {
  if (is_error_rsp(error_info)) {
    spdlog::error("[XtpMdApi::OnQueryAllTickers] {}", error_info->error_msg);
    error();
    return;
  }

  if (ticker_info && (ticker_info->ticker_type == XTP_TICKER_TYPE_STOCK ||
                      ticker_info->ticker_type == XTP_TICKER_TYPE_FUND)) {
    spdlog::debug("[XtpMdApi::OnQueryAllTickers] {}, {}", ticker_info->ticker,
                  ticker_info->ticker_name);
    Contract contract{};
    contract.ticker = ticker_info->ticker;
    contract.exchange = ft_exchange_type(ticker_info->exchange_id);
    contract.name = ticker_info->ticker_name;
    contract.price_tick = ticker_info->price_tick;
    contract.long_margin_rate = 1.0;
    contract.short_margin_rate = 1.0;
    if (ticker_info->ticker_type == XTP_TICKER_TYPE_STOCK)
      contract.product_type = ProductType::STOCK;
    else if (ticker_info->ticker_type == XTP_TICKER_TYPE_FUND)
      contract.product_type = ProductType::FUND;
    contract.size = 1;
    engine_->on_query_contract(&contract);
  }

  if (is_last) done();
}

void XtpMdApi::OnDepthMarketData(XTPMD* market_data, int64_t bid1_qty[],
                                 int32_t bid1_count, int32_t max_bid1_count,
                                 int64_t ask1_qty[], int32_t ask1_count,
                                 int32_t max_ask1_count) {
  if (!market_data) {
    spdlog::warn("[XtpMdApi::OnDepthMarketData] nullptr");
    return;
  }

  auto contract = ContractTable::get_by_ticker(market_data->ticker);
  if (!contract) {
    spdlog::warn("[XtpMdApi::OnDepthMarketData] {} not found int ContractTable",
                 market_data->ticker);
    return;
  }

  TickData tick{};
  tick.ticker_index = contract->index;

  // market_data->data_time
  // strptime(md->UpdateTime, "%H:%M:%S", &_tm);
  // tick.time_sec = _tm.tm_sec + _tm.tm_min * 60 + _tm.tm_hour * 3600;
  // tick.time_ms = md->UpdateMillisec;

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

  spdlog::debug(
      "[XtpMdApi::OnRtnDepthMarketData] Ticker: {}, Time MS: {}, "
      "LastPrice: {:.2f}, Volume: {}, Turnover: {}, Open Interest: {}",
      market_data->ticker, tick.time_ms, tick.last_price, tick.volume,
      tick.turnover, tick.open_interest);

  engine_->on_tick(&tick);
}

}  // namespace ft
