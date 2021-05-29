// Copyright [2020-2021] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "trader/risk/risk_rule.h"

namespace ft {

RiskRuleCtorMap& __GetRiskRuleCtorMap() {
  static RiskRuleCtorMap map;
  return map;
}

std::shared_ptr<RiskRule> CreateRiskRule(const std::string& risk_rule_name) {
  auto& map = __GetRiskRuleCtorMap();
  auto it = map.find(risk_rule_name);
  if (it == map.end()) {
    return nullptr;
  }
  return (*it->second)();
}

}  // namespace ft
