// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "risk_management/risk_manager.h"

#include "risk_management/etf_arbitrage/arbitrage_manager.h"
#include "risk_management/fund_manager.h"
#include "risk_management/no_self_trade.h"
#include "risk_management/position_manager.h"
#include "risk_management/strategy_notifier.h"
#include "risk_management/throttle_rate_limit.h"

namespace ft {

RiskManager::RiskManager() {}

bool RiskManager::init(const Config& config, Account* account,
                       Portfolio* portfolio,
                       std::map<uint64_t, Order>* order_map,
                       const MdSnapshot* md_snapshot) {
  add_rule(std::make_shared<FundManager>());
  add_rule(std::make_shared<PositionManager>());
  add_rule(std::make_shared<NoSelfTradeRule>());
  add_rule(std::make_shared<ThrottleRateLimit>());
  add_rule(std::make_shared<StrategyNotifier>());
  if (config.api == "xtp") add_rule(std::make_shared<ArbitrageManager>());

  for (auto& rule : rules_) {
    if (!rule->init(config, account, portfolio, order_map, md_snapshot))
      return false;
  }

  return true;
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

void RiskManager::on_order_accepted(const Order* order) {
  for (auto& rule : rules_) rule->on_order_accepted(order);
}

void RiskManager::on_order_traded(const Order* order,
                                  const OrderTradedRsp* trade) {
  for (auto& rule : rules_) rule->on_order_traded(order, trade);
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
