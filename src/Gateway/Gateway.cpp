// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "Core/Gateway.h"

#include <functional>
#include <map>

#include "Gateway/Ctp/CtpGateway.h"

namespace ft {

std::map<std::string, __GATEWAY_CREATE_FUNC>& __get_api_map() {
  static std::map<std::string, __GATEWAY_CREATE_FUNC> type_map;

  return type_map;
}

Gateway* create_gateway(const std::string& name,
                        TradingEngineInterface* engine) {
  auto& type_map = __get_api_map();
  auto iter = type_map.find(name);
  if (iter == type_map.end()) return nullptr;
  return iter->second(engine);
}

void destroy_api(Gateway* api) {
  if (api) delete api;
}

REGISTER_GATEWAY("ctp", CtpGateway);

}  // namespace ft
