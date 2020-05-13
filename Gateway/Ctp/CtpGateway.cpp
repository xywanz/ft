// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "Ctp/CtpGateway.h"

#include <ThostFtdcMdApi.h>
#include <ThostFtdcTraderApi.h>
#include <spdlog/spdlog.h>

#include "ContractTable.h"
#include "Core/Constants.h"

namespace ft {

CtpGateway::CtpGateway(TradingEngineInterface *engine)
    : Gateway(engine),
      trade_api_(new CtpTradeApi(engine)),
      md_api_(new CtpMdApi(engine)) {}

CtpGateway::~CtpGateway() {}

bool CtpGateway::login(const LoginParams &params) {
  if (params.broker_id().size() > sizeof(TThostFtdcBrokerIDType) ||
      params.broker_id().empty() ||
      params.investor_id().size() > sizeof(TThostFtdcUserIDType) ||
      params.investor_id().empty() ||
      params.passwd().size() > sizeof(TThostFtdcPasswordType) ||
      params.passwd().empty() || params.front_addr().empty()) {
    spdlog::error("[CtpGateway::login] Failed. Invalid login params");
    return false;
  }

  if (!params.front_addr().empty()) {
    if (!trade_api_->login(params)) {
      spdlog::error("[CtpGateway::login] Failed to login into the counter");
      return false;
    }
  }

  if (!params.md_server_addr().empty()) {
    if (!md_api_->login(params)) {
      spdlog::error("[CtpGateway::login] Failed to login into the md server");
      return false;
    }
  }

  return true;
}

void CtpGateway::logout() {
  trade_api_->logout();
  md_api_->logout();
}

uint64_t CtpGateway::send_order(const OrderReq *order) {
  return trade_api_->send_order(order);
}

bool CtpGateway::cancel_order(uint64_t order_id) {
  return trade_api_->cancel_order(order_id);
}

bool CtpGateway::query_contract(const std::string &ticker) {
  return trade_api_->query_contract(ticker);
}

bool CtpGateway::query_contracts() { return trade_api_->query_contracts(); }

bool CtpGateway::query_position(const std::string &ticker) {
  return trade_api_->query_position(ticker);
}

bool CtpGateway::query_positions() { return trade_api_->query_positions(); }

bool CtpGateway::query_account() {
  return trade_api_->query_account();
}

bool CtpGateway::query_orders() {
  return trade_api_->query_orders();
}

bool CtpGateway::query_trades() {
  return trade_api_->query_trades();
}

bool CtpGateway::query_margin_rate(const std::string &ticker) {
  return trade_api_->query_margin_rate(ticker);
}

}  // namespace ft
