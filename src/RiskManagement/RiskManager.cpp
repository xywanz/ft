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

int RiskManager::check_order_req(const OrderReq* order) {
  int error_code;

  for (auto& rule : rules_) {
    error_code = rule->check_order_req(order);
    if (error_code != NO_ERROR) return error_code;
  }

  return NO_ERROR;
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
