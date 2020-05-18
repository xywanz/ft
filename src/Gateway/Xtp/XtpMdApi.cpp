// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "Gateway/Xtp/XtpMdApi.h"

#include <spdlog/spdlog.h>

namespace ft {

XtpMdApi::XtpMdApi(TradingEngineInterface* engine) : engine_(engine) {}

XtpMdApi::~XtpMdApi() {
  error();
  logout();
}

bool XtpMdApi::login(const LoginParams& params) {
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
    int ret = sscanf(params.md_server_addr().c_str(), "%[^:]://%[^:]:%d",
                     protocol, ip, &port);
    assert(ret == 3);
  } catch (...) {
    assert(false);
  }

  XTP_PROTOCOL_TYPE sock_type = XTP_PROTOCOL_TCP;
  if (strcmp(protocol, "udp") == 0) sock_type = XTP_PROTOCOL_UDP;

  quote_api_->RegisterSpi(this);
  if (quote_api_->Login(ip, port, params.investor_id().c_str(),
                        params.passwd().c_str(), sock_type) != 0) {
    spdlog::error("[XtpMdApi::login] Failed to login: {}",
                  quote_api_->GetApiLastError()->error_msg);
    return false;
  }

  spdlog::debug("[XtpMdApi::login] Success");
  is_logon_ = true;
  return true;
}

void XtpMdApi::logout() {
  if (is_logon_) {
    quote_api_->Logout();
    is_logon_ = false;
  }
}

bool XtpMdApi::query_contract(const std::string& ticker) {
  spdlog::error("[XtpMdApi::query_contract] XTP不支持查询单个合约信息");
  return false;
}

bool XtpMdApi::query_contracts() {
  if (!is_logon_) {
    spdlog::error("[XtpMdApi::query_contracts] 未登录到quote服务器");
    return false;
  }

  std::unique_lock<std::mutex> lock(query_mutex_);
  reset_sync();
  if (quote_api_->QueryAllTickers(XTP_EXCHANGE_SH) != 0) {
    spdlog::error("[XtpMdApi::query_contract] Failed to query SH stocks");
    return false;
  }
  if (!wait_sync()) {
    spdlog::error("[XtpMdApi::query_contract] Failed to query SH stocks");
    return false;
  }

  reset_sync();
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

  if (ticker_info && ticker_info->ticker_type == XTP_TICKER_TYPE_STOCK) {
    spdlog::debug("[XtpMdApi::OnQueryAllTickers] {}, {}", ticker_info->ticker,
                  ticker_info->ticker_name);
    Contract contract{};
    contract.symbol = ticker_info->ticker;
    contract.exchange = ft_exchange_type(ticker_info->exchange_id);
    contract.ticker = to_ticker(contract.symbol, contract.exchange);
    contract.name = ticker_info->ticker_name;
    contract.price_tick = ticker_info->price_tick;
    contract.product_type = ProductType::STOCK;
    contract.size = 1;
    engine_->on_query_contract(&contract);
  }

  if (is_last) done();
}

}  // namespace ft
