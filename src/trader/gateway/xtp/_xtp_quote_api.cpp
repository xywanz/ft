// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "trader/gateway/xtp/_xtp_quote_api.h"

#include <thread>
#include <utility>
#include <vector>

#include "ft/base/contract_table.h"
#include "ft/base/log.h"
#include "ft/utils/misc.h"
#include "trader/gateway/xtp/xtp_gateway.h"

namespace ft {

XtpQuoteApi::XtpQuoteApi(XtpGateway* gateway) : gateway_(gateway) {}

XtpQuoteApi::~XtpQuoteApi() { Logout(); }

bool XtpQuoteApi::Login(const GatewayConfig& config) {
  uint32_t seed = time(nullptr);
  uint8_t client_id = rand_r(&seed) & 0xff;
  quote_api_.reset(XTP::API::QuoteApi::CreateQuoteApi(client_id, "."));
  if (!quote_api_) {
    LOG_ERROR("[XtpQuoteApi::Login] Failed to CreateQuoteApi");
    return false;
  }

  char protocol[32]{};
  char ip[32]{};
  int port = 0;

  try {
    int ret = sscanf(config.quote_server_address.c_str(), "%[^:]://%[^:]:%d", protocol, ip, &port);
    if (ret != 3) {
      LOG_ERROR("FAILED to parse server address: {}", config.quote_server_address);
      return false;
    }
  } catch (...) {
    LOG_ERROR("FAILED to parse server address: {}", config.quote_server_address);
    return false;
  }

  XTP_PROTOCOL_TYPE sock_type = XTP_PROTOCOL_TCP;
  if (strcmp(protocol, "udp") == 0) sock_type = XTP_PROTOCOL_UDP;

  quote_api_->RegisterSpi(this);
  if (quote_api_->Login(ip, port, config.investor_id.c_str(), config.password.c_str(), sock_type) !=
      0) {
    LOG_ERROR("[XtpQuoteApi::Login] Failed to Login: {}", quote_api_->GetApiLastError()->error_msg);
    return false;
  }

  LOG_DEBUG("[XtpQuoteApi::Login] Success");
  return true;
}

void XtpQuoteApi::Logout() {
  if (quote_api_) {
    quote_api_->Logout();
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
      LOG_ERROR("[XtpQuoteApi::Login] 无法订阅行情");
      return false;
    }
  }

  if (sub_list_sz.size() > 0) {
    if (quote_api_->SubscribeMarketData(sub_list_sz.data(), sub_list_sz.size(), XTP_EXCHANGE_SZ) !=
        0) {
      LOG_ERROR("[XtpQuoteApi::Login] 无法订阅行情");
      return false;
    }
  }

  return true;
}

bool XtpQuoteApi::QueryContracts() {
  query_count_ = 0;
  qry_contract_res_.clear();

  if (quote_api_->QueryAllTickers(XTP_EXCHANGE_SH) != 0) {
    LOG_ERROR("[XtpQuoteApi::QueryContract] Failed to query SH stocks");
    return false;
  }

  std::this_thread::sleep_for(std::chrono::seconds(1));

  if (quote_api_->QueryAllTickers(XTP_EXCHANGE_SZ) != 0) {
    LOG_ERROR("[XtpQuoteApi::QueryContract] Failed to query SZ stocks");
    return false;
  }

  return true;
}

void XtpQuoteApi::OnQueryAllTickers(XTPQSI* ticker_info, XTPRI* error_info, bool is_last) {
  if (is_error_rsp(error_info)) {
    LOG_ERROR("[XtpQuoteApi::OnQueryAllTickers] {}", error_info->error_msg);
    gateway_->OnQueryContractEnd();
    qry_contract_res_.clear();
    qry_contract_res_.shrink_to_fit();
    return;
  }

  if (ticker_info && (ticker_info->ticker_type == XTP_TICKER_TYPE_STOCK ||
                      ticker_info->ticker_type == XTP_TICKER_TYPE_FUND)) {
    LOG_DEBUG("[XtpQuoteApi::OnQueryAllTickers] {}, {}", ticker_info->ticker,
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

    qry_contract_res_.emplace_back(std::move(contract));
    gateway_->OnQueryContract(contract);
  }

  if (is_last) {
    ++query_count_;
    if (query_count_ == 2) {
      for (auto& contract : qry_contract_res_) {
        gateway_->OnQueryContract(contract);
      }
      gateway_->OnQueryContractEnd();
    }

    qry_contract_res_.clear();
    qry_contract_res_.shrink_to_fit();
  }
}

void XtpQuoteApi::OnDepthMarketData(XTPMD* market_data, int64_t bid1_qty[], int32_t bid1_count,
                                    int32_t max_bid1_count, int64_t ask1_qty[], int32_t ask1_count,
                                    int32_t max_ask1_count) {
  if (!market_data) {
    LOG_WARN("[XtpQuoteApi::OnDepthMarketData] nullptr");
    return;
  }

  timespec ts;
  clock_gettime(CLOCK_REALTIME, &ts);

  auto contract = ContractTable::get_by_ticker(market_data->ticker);
  if (!contract) {
    LOG_TRACE("[XtpQuoteApi::OnDepthMarketData] {} not found int ContractTable",
              market_data->ticker);
    return;
  }

  dt_converter_.UpdateDate(market_data->data_time);

  TickData tick{};
  tick.source = MarketDataSource::kXTP;
  tick.ticker_id = contract->ticker_id;
  tick.local_timestamp_us = ts.tv_sec * 1000000UL + ts.tv_nsec / 1000UL;
  tick.exchange_timestamp_us = dt_converter_.GetExchTimeStamp(market_data->data_time);

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

  LOG_TRACE(
      "[XtpQuoteApi::OnRtnDepthMarketData] {}, ExchangeTimeStamp: {}, LastPrice:{:.2f}, "
      "Volume:{}, Turnover:{}, Open Interest:{}",
      market_data->ticker, tick.exchange_timestamp_us, tick.last_price, tick.volume, tick.turnover,
      tick.open_interest);

  gateway_->OnTick(tick);
}

}  // namespace ft
