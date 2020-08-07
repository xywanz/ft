// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "virtual_gateway.h"

#include <spdlog/spdlog.h>

#include "cep/data/contract_table.h"

namespace ft {

VirtualGateway::VirtualGateway() { virtual_api_.set_spi(this); }

bool VirtualGateway::login(OMSInterface* oms, const Config& config) {
  oms_ = oms;
  virtual_api_.start_quote_server();
  virtual_api_.start_trade_server();
  spdlog::info("[VirtualGateway::login] Virtual API v" VIRTUAL_GATEWAY_VERSION);
  return true;
}

void VirtualGateway::logout() {}

bool VirtualGateway::send_order(const OrderRequest& order) {
  VirtualOrderReq req{};
  req.oms_order_id = order.oms_order_id;
  req.tid = order.contract->tid;
  req.direction = order.direction;
  req.offset = order.offset;
  req.type = order.type;
  req.volume = order.volume;
  req.price = order.price;

  return virtual_api_.insert_order(&req);
}

bool VirtualGateway::cancel_order(uint64_t oms_order_id) {
  return virtual_api_.cancel_order(oms_order_id);
}

bool VirtualGateway::query_contracts(std::vector<Contract>* result) {
  return true;
}

bool VirtualGateway::query_positions(std::vector<Position>* result) {
  return true;
}

bool VirtualGateway::query_account(Account* result) {
  result->account_id = 1234;
  result->total_asset = 100000000;
  return true;
}

bool VirtualGateway::query_trades(std::vector<Trade>* result) { return true; }

bool VirtualGateway::query_margin_rate(const std::string& ticker) {
  return true;
}

bool VirtualGateway::query_commision_rate(const std::string& ticker) {
  return true;
}

void VirtualGateway::on_order_accepted(uint64_t oms_order_id) {
  OrderAcceptance rsp = {oms_order_id, oms_order_id};
  oms_->on_order_accepted(&rsp);
}

void VirtualGateway::on_order_traded(uint64_t oms_order_id, int traded,
                                     double price) {
  Trade rsp{};
  rsp.oms_order_id = oms_order_id;
  rsp.order_id = oms_order_id;
  rsp.volume = traded;
  rsp.price = price;
  oms_->on_order_traded(&rsp);
}

void VirtualGateway::on_order_canceled(uint64_t order_id, int canceled) {
  OrderCancellation rsp = {order_id, canceled};
  oms_->on_order_canceled(&rsp);
}

void VirtualGateway::on_tick(TickData* tick) { oms_->on_tick(tick); }

}  // namespace ft
