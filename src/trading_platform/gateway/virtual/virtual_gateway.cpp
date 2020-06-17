// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "gateway/virtual/virtual_gateway.h"

#include <spdlog/spdlog.h>

#include "core/contract_table.h"

namespace ft {

VirtualGateway::VirtualGateway() { virtual_api_.set_spi(this); }

bool VirtualGateway::login(TradingEngineInterface* engine,
                           const Config& config) {
  engine_ = engine;
  virtual_api_.start_quote_server();
  virtual_api_.start_trade_server();
  spdlog::info("[VirtualGateway::login] Virtual API v" VIRTUAL_GATEWAY_VERSION);
  return true;
}

void VirtualGateway::logout() {}

bool VirtualGateway::send_order(const OrderReq& order) {
  VirtualOrderReq req{};
  req.engine_order_id = order.engine_order_id;
  req.ticker_index = order.contract->index;
  req.direction = order.direction;
  req.offset = order.offset;
  req.type = order.type;
  req.volume = order.volume;
  req.price = order.price;

  return virtual_api_.insert_order(&req);
}

bool VirtualGateway::cancel_order(uint64_t engine_order_id) {
  return virtual_api_.cancel_order(engine_order_id);
}

bool VirtualGateway::query_contract(const std::string& ticker,
                                    const std::string& exchange) {
  return true;
}

bool VirtualGateway::query_contracts() { return true; }

bool VirtualGateway::query_position(const std::string& ticker) { return true; }

bool VirtualGateway::query_positions() { return true; }

bool VirtualGateway::query_account() {
  Account account{};
  account.account_id = 1234;
  account.total_asset = 100000000;
  engine_->on_query_account(&account);
  return true;
}

bool VirtualGateway::query_trades() { return true; }

bool VirtualGateway::query_margin_rate(const std::string& ticker) {
  return true;
}

bool VirtualGateway::query_commision_rate(const std::string& ticker) {
  return true;
}

void VirtualGateway::on_order_accepted(uint64_t engine_order_id) {
  OrderAcceptedRsp rsp = {engine_order_id, engine_order_id};
  engine_->on_order_accepted(&rsp);
}

void VirtualGateway::on_order_traded(uint64_t engine_order_id, int traded,
                                     double price) {
  OrderTradedRsp rsp{};
  rsp.engine_order_id = engine_order_id;
  rsp.order_id = engine_order_id;
  rsp.volume = traded;
  rsp.price = price;
  engine_->on_order_traded(&rsp);
}

void VirtualGateway::on_order_canceled(uint64_t order_id, int canceled) {
  OrderCanceledRsp rsp = {order_id, canceled};
  engine_->on_order_canceled(&rsp);
}

void VirtualGateway::on_tick(TickData* tick) { engine_->on_tick(tick); }

}  // namespace ft
