// Copyright [2020-2021] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "trader/gateway/gateway.h"

#include "trader/gateway/backtest/backtest_gateway.h"
#include "trader/gateway/ctp/ctp_gateway.h"
#include "trader/gateway/stub/stub_gateway.h"
#include "trader/gateway/xtp/xtp_gateway.h"

namespace ft {

using CreateGatewayFn = std::shared_ptr<Gateway> (*)();
using GatewayCtorMap = std::map<std::string, CreateGatewayFn>;

GatewayCtorMap& __GetGatewayCtorMap() {
  static GatewayCtorMap map;
  return map;
}

std::shared_ptr<Gateway> CreateGateway(const std::string& name) {
  auto& map = __GetGatewayCtorMap();
  auto it = map.find(name);
  if (it == map.end()) {
    return nullptr;
  } else {
    return (*it->second)();
  }
}

#define REGISTER_GATEWAY(name, type)                                \
  static std::shared_ptr<::ft::Gateway> __CreateGateway##type() {   \
    return std::make_shared<type>();                                \
  }                                                                 \
  static const bool __is_##type##_registered [[gnu::unused]] = [] { \
    ::ft::__GetGatewayCtorMap()[name] = &__CreateGateway##type;     \
    return true;                                                    \
  }();

REGISTER_GATEWAY("ctp", CtpGateway);
REGISTER_GATEWAY("xtp", XtpGateway);
REGISTER_GATEWAY("backtest", BacktestGateway);
REGISTER_GATEWAY("stub", StubGateway);

}  // namespace ft
