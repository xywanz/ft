// Copyright [2020-present] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "trader/gateway/backtest/match_engine/match_engine.h"

#include "trader/gateway/backtest/match_engine/advanced_match_engine.h"
#include "trader/gateway/backtest/match_engine/simple_match_engine.h"

namespace ft {

using CreateMatchEngineFn = std::shared_ptr<MatchEngine> (*)();
using MatchEngineCtorMap = std::map<std::string, CreateMatchEngineFn>;

MatchEngineCtorMap& __GetMatchEngineCtorMap() {
  static MatchEngineCtorMap map;
  return map;
}

std::shared_ptr<MatchEngine> CreateMatchEngine(const std::string& name) {
  auto& map = __GetMatchEngineCtorMap();
  auto it = map.find(name);
  if (it == map.end()) {
    return nullptr;
  } else {
    return (*it->second)();
  }
}

#define REGISTER_MATCH_ENGINE(name, type)                                 \
  static std::shared_ptr<::ft::MatchEngine> __CreateMatchEngine##type() { \
    return std::make_shared<type>();                                      \
  }                                                                       \
  static const bool __is_##type##_registered [[gnu::unused]] = [] {       \
    ::ft::__GetMatchEngineCtorMap()[name] = &__CreateMatchEngine##type;   \
    return true;                                                          \
  }();

REGISTER_MATCH_ENGINE("ft.match_engine.simple", SimpleMatchEngine);
REGISTER_MATCH_ENGINE("ft.match_engine.advanced", AdvancedMatchEngine);

}  // namespace ft
