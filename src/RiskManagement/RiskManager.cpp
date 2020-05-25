// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "RiskManagement/RiskManager.h"

#include "RiskManagement/AvailablePosCheck.h"
#include "RiskManagement/NoSelfTrade.h"
#include "RiskManagement/ThrottleRateLimit.h"

namespace ft {

RiskManager::RiskManager(const PositionManager* pos_mgr) : pos_mgr_(pos_mgr) {
  // 先硬编码吧
  add_rule(std::make_shared<AvailablePosCheck>(pos_mgr));
  add_rule(std::make_shared<NoSelfTradeRule>());
  add_rule(std::make_shared<ThrottleRateLimit>(1000, 2, 100));
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

void RiskManager::on_order_traded(uint64_t order_id, int this_traded,
                                  double traded_price) {
  for (auto& rule : rules_)
    rule->on_order_traded(order_id, this_traded, traded_price);
}

void RiskManager::on_order_completed(uint64_t order_id, int error_code) {
  for (auto& rule : rules_) rule->on_order_completed(order_id, error_code);
}

}  // namespace ft
