// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "trader/gateway/ctp/ctp_gateway.h"

#include "ThostFtdcMdApi.h"
#include "ThostFtdcTraderApi.h"
#include "ft/base/contract_table.h"
#include "ft/base/log.h"

namespace ft {

CtpGateway::CtpGateway() {}

CtpGateway::~CtpGateway() {}

bool CtpGateway::Init(const GatewayConfig &config) {
  if (config.broker_id.size() > sizeof(TThostFtdcBrokerIDType) || config.broker_id.empty() ||
      config.investor_id.size() > sizeof(TThostFtdcUserIDType) || config.investor_id.empty() ||
      config.password.size() > sizeof(TThostFtdcPasswordType) || config.password.empty()) {
    LOG_ERROR("[CtpGateway::Login] Failed. Invalid Login config");
    return false;
  }

  if (config.trade_server_address.empty() && config.quote_server_address.empty()) {
    LOG_WARN("[CtpGateway::Login] 交易柜台和行情服务器地址都未设置");
    return false;
  }

  if (!config.trade_server_address.empty()) {
    LOG_DEBUG("[CtpGateway::Login] Login into trading server");
    trade_api_ = std::make_unique<CtpTradeApi>(this);
    if (!trade_api_->Login(config)) {
      LOG_ERROR("[CtpGateway::Login] Failed to Login into the counter");
      return false;
    }
  }

  if (!config.quote_server_address.empty()) {
    LOG_DEBUG("[CtpGateway::Login] Login into market data server");
    quote_api_ = std::make_unique<CtpQuoteApi>(this);
    if (!quote_api_->Login(config)) {
      LOG_ERROR("[CtpGateway::Login] Failed to Login into the md server");
      return false;
    }
  }

  return true;
}

void CtpGateway::Logout() {
  trade_api_->Logout();
  quote_api_->Logout();
}

bool CtpGateway::SendOrder(const OrderRequest &order, uint64_t *privdata_ptr) {
  return trade_api_->SendOrder(order, privdata_ptr);
}

bool CtpGateway::CancelOrder(uint64_t order_id, uint64_t privdata) {
  return trade_api_->CancelOrder(order_id, privdata);
}

bool CtpGateway::Subscribe(const std::vector<std::string> &sub_list) {
  return quote_api_->Subscribe(sub_list);
}

bool CtpGateway::QueryContracts() { return trade_api_->QueryContracts(); }

bool CtpGateway::QueryPositions() { return trade_api_->QueryPositions(); }

bool CtpGateway::QueryAccount() { return trade_api_->QueryAccount(); }

bool CtpGateway::QueryTrades() { return trade_api_->QueryTrades(); }

}  // namespace ft
