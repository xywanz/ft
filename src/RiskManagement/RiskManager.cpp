// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "RiskManagement/RiskManager.h"

#include "RiskManagement/NoSelfTrade.h"
#include "RiskManagement/VelocityLimit.h"

namespace ft {

RiskManager::RiskManager() {
  // 先硬编码吧
  add_rule(std::make_shared<VelocityLimit>(1000, 5, 100));
  add_rule(std::make_shared<NoSelfTradeRule>());
}

void RiskManager::add_rule(std::shared_ptr<RiskRuleInterface> rule) {
  rules_.emplace_back(rule);
}

bool RiskManager::check_order_req(const OrderReq* order) {
  for (auto& rule : rules_) {
    if (!rule->check_order_req(order)) return false;
  }

  return true;
}

void RiskManager::on_order_sent(uint64_t order_id) {
  for (auto& rule : rules_) rule->on_order_sent(order_id);
}

void RiskManager::on_order_traded(uint64_t order_id, int64_t this_traded,
                                  double traded_price) {
  for (auto& rule : rules_)
    rule->on_order_traded(order_id, this_traded, traded_price);
}

void RiskManager::on_order_completed(uint64_t order_id) {
  for (auto& rule : rules_) rule->on_order_completed(order_id);
}

}  // namespace ft
