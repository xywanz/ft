// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "gateway/virtual/virtual_gateway.h"

#include <spdlog/spdlog.h>

#include "component/contract_table/contract_table.h"

namespace ft {

VirtualGateway::VirtualGateway() { virtual_api_.set_spi(this); }

bool VirtualGateway::Login(BaseOrderManagementSystem* oms, const Config& config) {
  oms_ = oms;
  virtual_api_.StartQuoteServer();
  virtual_api_.StartTradeServer();
  spdlog::info("[VirtualGateway::Login] Virtual API v" VIRTUAL_GATEWAY_VERSION);
  return true;
}

void VirtualGateway::Logout() {}

bool VirtualGateway::SendOrder(const OrderRequest& order, uint64_t* privdata_ptr) {
  (void)privdata_ptr;

  VirtualOrderRequest req{};
  req.oms_order_id = order.order_id;
  req.ticker_id = order.contract->ticker_id;
  req.direction = order.direction;
  req.offset = order.offset;
  req.type = order.type;
  req.volume = order.volume;
  req.price = order.price;

  return virtual_api_.InsertOrder(&req);
}

bool VirtualGateway::CancelOrder(uint64_t oms_order_id, uint64_t privdata) {
  (void)privdata;
  return virtual_api_.CancelOrder(oms_order_id);
}

bool VirtualGateway::QueryContractList(std::vector<Contract>* result) { return true; }

bool VirtualGateway::QueryPositionList(std::vector<Position>* result) { return true; }

bool VirtualGateway::QueryAccount(Account* result) { return virtual_api_.QueryAccount(result); }

bool VirtualGateway::QueryTradeList(std::vector<Trade>* result) { return true; }

bool VirtualGateway::QueryMarginRate(const std::string& ticker) { return true; }

bool VirtualGateway::QueryCommisionRate(const std::string& ticker) { return true; }

void VirtualGateway::OnOrderAccepted(uint64_t oms_order_id) {
  OrderAcceptance rsp = {oms_order_id};
  oms_->OnOrderAccepted(&rsp);
}

void VirtualGateway::OnOrderTraded(uint64_t oms_order_id, int traded, double price) {
  Trade rsp{};
  rsp.order_id = oms_order_id;
  rsp.volume = traded;
  rsp.price = price;
  rsp.trade_type = TradeType::SECONDARY_MARKET;
  oms_->OnOrderTraded(&rsp);
}

void VirtualGateway::OnOrderCanceled(uint64_t order_id, int canceled) {
  OrderCancellation rsp = {order_id, canceled};
  oms_->OnOrderCanceled(&rsp);
}

void VirtualGateway::OnTick(TickData* tick) { oms_->OnTick(tick); }

REGISTER_GATEWAY(::ft::VirtualGateway);

}  // namespace ft
