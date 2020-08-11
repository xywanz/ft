// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "ctp_gateway.h"

#include <ThostFtdcMdApi.h>
#include <ThostFtdcTraderApi.h>
#include <spdlog/spdlog.h>

#include "cep/data/constants.h"
#include "cep/data/contract_table.h"

namespace ft {

CtpGateway::CtpGateway() {}

CtpGateway::~CtpGateway() {}

bool CtpGateway::login(OMSInterface *oms, const Config &config) {
  if (config.broker_id.size() > sizeof(TThostFtdcBrokerIDType) ||
      config.broker_id.empty() ||
      config.investor_id.size() > sizeof(TThostFtdcUserIDType) ||
      config.investor_id.empty() ||
      config.password.size() > sizeof(TThostFtdcPasswordType) ||
      config.password.empty()) {
    spdlog::error("[CtpGateway::login] Failed. Invalid login config");
    return false;
  }

  if (config.trade_server_address.empty() &&
      config.quote_server_address.empty()) {
    spdlog::warn("[CtpGateway::login] 交易柜台和行情服务器地址都未设置");
    return false;
  }

  if (!config.trade_server_address.empty()) {
    spdlog::debug("[CtpGateway::login] Login into trading server");
    trade_api_ = std::make_unique<CtpTradeApi>(oms);
    if (!trade_api_->login(config)) {
      spdlog::error("[CtpGateway::login] Failed to login into the counter");
      return false;
    }
  }

  if (!config.quote_server_address.empty()) {
    spdlog::debug("[CtpGateway::login] Login into market data server");
    quote_api_ = std::make_unique<CtpQuoteApi>(oms);
    if (!quote_api_->login(config)) {
      spdlog::error("[CtpGateway::login] Failed to login into the md server");
      return false;
    }
  }

  return true;
}

void CtpGateway::logout() {
  trade_api_->logout();
  quote_api_->logout();
}

bool CtpGateway::send_order(const OrderRequest &order, uint64_t *privdata_ptr) {
  return trade_api_->send_order(order, privdata_ptr);
}

bool CtpGateway::cancel_order(uint64_t order_id, uint64_t privdata) {
  return trade_api_->cancel_order(order_id, privdata);
}

bool CtpGateway::subscribe(const std::vector<std::string> &sub_list) {
  return quote_api_->subscribe(sub_list);
}

bool CtpGateway::query_contracts(std::vector<Contract> *result) {
  return trade_api_->query_contracts(result);
}

bool CtpGateway::query_positions(std::vector<Position> *result) {
  return trade_api_->query_positions(result);
}

bool CtpGateway::query_account(Account *result) {
  return trade_api_->query_account(result);
}

bool CtpGateway::query_trades(std::vector<Trade> *result) {
  return trade_api_->query_trades(result);
}

bool CtpGateway::query_margin_rate(const std::string &ticker) {
  return trade_api_->query_margin_rate(ticker);
}

}  // namespace ft
