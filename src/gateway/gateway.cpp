// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "gateway/gateway.h"

#include <functional>
#include <map>

#include "gateway/ctp/ctp_gateway.h"
#include "gateway/ocg_bss/broker/broker.h"
#include "gateway/virtual/virtual_gateway.h"
#include "gateway/xtp/xtp_gateway.h"

namespace ft {

std::map<std::string, __GATEWAY_CREATE_FUNC>& __GetApiMap() {
  static std::map<std::string, __GATEWAY_CREATE_FUNC> type_map;

  return type_map;
}

Gateway* CreateGateway(const std::string& name) {
  auto& type_map = __GetApiMap();
  auto iter = type_map.find(name);
  if (iter == type_map.end()) return nullptr;
  return iter->second();
}

void destroy_api(Gateway* api) {
  if (api) delete api;
}

REGISTER_GATEWAY("ctp", CtpGateway);
REGISTER_GATEWAY("xtp", XtpGateway);
REGISTER_GATEWAY("virtual", VirtualGateway);
REGISTER_GATEWAY("ocg-bss", BssBroker);

}  // namespace ft
