// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "xtp_gateway.h"

#include <spdlog/spdlog.h>

namespace ft {

XtpGateway::XtpGateway() {}

bool XtpGateway::login(OMSInterface* oms, const Config& config) {
  if (config.trade_server_address.empty() &&
      config.quote_server_address.empty()) {
    spdlog::error("[XtpGateway::login] 交易柜台和行情服务器地址都未设置");
    return false;
  }

  if (!config.trade_server_address.empty()) {
    trade_api_ = std::make_unique<XtpTradeApi>(oms);
    if (!trade_api_->login(config)) {
      spdlog::error("[XtpGateway::login] Failed to login into the counter");
      return false;
    }
  }

  if (!config.quote_server_address.empty()) {
    quote_api_ = std::make_unique<XtpQuoteApi>(oms);
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

bool XtpGateway::send_order(const OrderRequest& order) {
  return trade_api_->send_order(order);
}

bool XtpGateway::cancel_order(uint64_t order_id) {
  return trade_api_->cancel_order(order_id);
}

bool XtpGateway::query_contracts(std::vector<Contract>* result) {
  return quote_api_->query_contracts(result);
}

bool XtpGateway::query_account(Account* result) {
  return trade_api_->query_account(result);
}

bool XtpGateway::query_positions(std::vector<Position>* result) {
  return trade_api_->query_positions(result);
}

bool XtpGateway::query_trades(std::vector<Trade>* result) {
  return trade_api_->query_trades(result);
}

bool XtpGateway::query_orders() { return trade_api_->query_orders(); }

}  // namespace ft
