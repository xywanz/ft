// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "Gateway/Virtual/VirtualGateway.h"

#include <spdlog/spdlog.h>

namespace ft {

bool VirtualGateway::login(const LoginParams& params) {
  spdlog::info("[VirtualGateway::login] Virtual API v0.0.1");
  return true;
}

void VirtualGateway::logout() {}

bool VirtualGateway::send_order(const OrderReq* order) { return true; }

bool VirtualGateway::cancel_order(uint64_t order_id) { return false; }

bool VirtualGateway::query_contract(const std::string& ticker) { return true; }

bool VirtualGateway::query_contracts() { return true; }

bool VirtualGateway::query_position(const std::string& ticker) { return true; }

bool VirtualGateway::query_positions() { return true; }

bool VirtualGateway::query_account() { return true; }

bool VirtualGateway::query_trades() { return true; }

bool VirtualGateway::query_margin_rate(const std::string& ticker) {
  return true;
}

bool VirtualGateway::query_commision_rate(const std::string& ticker) {
  return true;
}

}  // namespace ft
