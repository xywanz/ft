// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "gateway/xtp/xtp_gateway.h"

#include <spdlog/spdlog.h>

namespace ft {

XtpGateway::XtpGateway(TradingEngineInterface* engine)
    : engine_(engine),
      trade_api_(std::make_unique<XtpTradeApi>(engine)),
      quote_api_(std::make_unique<XtpQuoteApi>(engine)) {}

bool XtpGateway::login(const Config& config) {
  if (!config.trade_server_address.empty()) {
    if (!trade_api_->login(config)) {
      spdlog::error("[XtpGateway::login] Failed to login into the counter");
      return false;
    }
  }

  if (!config.quote_server_address.empty()) {
    if (!quote_api_->login(config)) {
      spdlog::error("[XtpGateway::login] Failed to login into the md server");
      return false;
    }
  }

  return true;
}

void XtpGateway::logout() {
  trade_api_->logout();
  quote_api_->logout();
}

bool XtpGateway::send_order(const OrderReq* order) {
  return trade_api_->send_order(order);
}

bool XtpGateway::cancel_order(uint64_t order_id) {
  return trade_api_->cancel_order(order_id);
}

bool XtpGateway::query_contract(const std::string& ticker,
                                const std::string& exchange) {
  return quote_api_->query_contract(ticker, exchange);
}

bool XtpGateway::query_contracts() { return quote_api_->query_contracts(); }

bool XtpGateway::query_account() { return trade_api_->query_account(); }

bool XtpGateway::query_position(const std::string& ticker) {
  return trade_api_->query_position(ticker);
}

bool XtpGateway::query_positions() { return trade_api_->query_positions(); }

bool XtpGateway::query_trades() { return trade_api_->query_trades(); }

bool XtpGateway::query_orders() { return trade_api_->query_orders(); }

}  // namespace ft
