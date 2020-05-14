// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "RiskManagement/RiskManager.h"

namespace ft {

void RiskManager::add_rule(std::shared_ptr<RiskRuleInterface> rule) {
  rules_.emplace_back(rule);
}

bool RiskManager::check_order_req(const OrderReq* order) {
  for (auto& rule : rules_) {
    if (!rule->check(order)) return false;
  }

  return true;
}

void RiskManager::on_order_traded(uint64_t order_id, int64_t this_traded,
                                  double traded_price) {}

void RiskManager::on_order_completed(uint64_t order_id) {}

}  // namespace ft
