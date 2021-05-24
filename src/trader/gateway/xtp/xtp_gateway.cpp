// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "trader/gateway/xtp/xtp_gateway.h"

#include "ft/base/contract_table.h"
#include "ft/base/log.h"

namespace ft {

XtpGateway::XtpGateway() {}

bool XtpGateway::Init(const GatewayConfig& config) {
  if (config.trade_server_address.empty() && config.quote_server_address.empty()) {
    LOG_ERROR("[XtpGateway::Login] 交易柜台和行情服务器地址都未设置");
    return false;
  }

  if (!config.trade_server_address.empty()) {
    trade_api_ = std::make_unique<XtpTradeApi>(this);
    if (!trade_api_->Login(config)) {
      LOG_ERROR("[XtpGateway::Login] Failed to Login into the counter");
      return false;
    }
  }

  if (!config.quote_server_address.empty()) {
    quote_api_ = std::make_unique<XtpQuoteApi>(this);
    if (!quote_api_->Login(config)) {
      LOG_ERROR("[XtpGateway::Login] Failed to Login into the md server");
      return false;
    }
  }

  return true;
}

void XtpGateway::Logout() {
  trade_api_->Logout();
  quote_api_->Logout();
}

bool XtpGateway::SendOrder(const OrderRequest& order, uint64_t* privdata_ptr) {
  return trade_api_->SendOrder(order, privdata_ptr);
}

bool XtpGateway::CancelOrder(uint64_t order_id, uint64_t privdata) {
  (void)order_id;
  return trade_api_->CancelOrder(privdata);
}

bool XtpGateway::Subscribe(const std::vector<std::string>& sub_list) {
  return quote_api_->Subscribe(sub_list);
}

bool XtpGateway::QueryContracts() { return quote_api_->QueryContracts(); }

bool XtpGateway::QueryAccount() { return trade_api_->QueryAccount(); }

bool XtpGateway::QueryPositions() { return trade_api_->QueryPositions(); }

bool XtpGateway::QueryTrades() { return trade_api_->QueryTrades(); }

}  // namespace ft
