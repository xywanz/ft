// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "RiskManagement/RiskManager.h"

namespace ft {

void RiskManager::add_rule(std::shared_ptr<RiskRuleInterface> rule) {
  rules_.emplace_back(rule);
}

bool RiskManager::check(const Order* order) {
  for (auto& rule : rules_) {
    if (!rule->check(order)) return false;
  }

  return true;
}

}  // namespace ft
