// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "RiskManagement/RiskManager.h"

#include "RiskManagement/FundManager.h"
#include "RiskManagement/NoSelfTrade.h"
#include "RiskManagement/PositionManager.h"
#include "RiskManagement/ThrottleRateLimit.h"

namespace ft {

RiskManager::RiskManager(Account* account, Portfolio* portfolio) {
  // 先硬编码吧
  add_rule(std::make_shared<FundManager>(account));
  add_rule(std::make_shared<PositionManager>(portfolio));
  add_rule(std::make_shared<NoSelfTradeRule>());
  add_rule(std::make_shared<ThrottleRateLimit>(1000, 2, 100));
}

void RiskManager::add_rule(std::shared_ptr<RiskRuleInterface> rule) {
  rules_.emplace_back(rule);
}

int RiskManager::check_order_req(const Order* order) {
  int error_code;

  for (auto& rule : rules_) {
    error_code = rule->check_order_req(order);
    if (error_code != NO_ERROR) return error_code;
  }

  return NO_ERROR;
}

void RiskManager::on_order_sent(const Order* order) {
  for (auto& rule : rules_) rule->on_order_sent(order);
}

void RiskManager::on_order_traded(const Order* order, int this_traded,
                                  double traded_price) {
  for (auto& rule : rules_)
    rule->on_order_traded(order, this_traded, traded_price);
}

void RiskManager::on_order_canceled(const Order* order, int canceled) {
  for (auto& rule : rules_) rule->on_order_canceled(order, canceled);
}

void RiskManager::on_order_rejected(const Order* order, int error_code) {
  for (auto& rule : rules_) rule->on_order_rejected(order, error_code);
}

void RiskManager::on_order_completed(const Order* order) {
  for (auto& rule : rules_) rule->on_order_completed(order);
}

}  // namespace ft
