// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "cep/rms/rms.h"

#include "cep/rms/common/fund_manager.h"
#include "cep/rms/common/no_self_trade.h"
#include "cep/rms/common/position_manager.h"
#include "cep/rms/common/strategy_notifier.h"
#include "cep/rms/common/throttle_rate_limit.h"
#include "cep/rms/etf/arbitrage_manager.h"

namespace ft {

RMS::RMS() {}

bool RMS::init(const Config& config, Account* account, Portfolio* portfolio,
               OrderMap* order_map, const MdSnapshot* md_snapshot) {
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

void RMS::add_rule(std::shared_ptr<RiskRule> rule) {
  rules_.emplace_back(rule);
}

int RMS::check_order_req(const Order* order) {
  int error_code;

  for (auto& rule : rules_) {
    error_code = rule->check_order_req(order);
    if (error_code != NO_ERROR) return error_code;
  }

  return NO_ERROR;
}

void RMS::on_order_sent(const Order* order) {
  for (auto& rule : rules_) rule->on_order_sent(order);
}

void RMS::on_order_accepted(const Order* order) {
  for (auto& rule : rules_) rule->on_order_accepted(order);
}

void RMS::on_order_traded(const Order* order, const Trade* trade) {
  for (auto& rule : rules_) rule->on_order_traded(order, trade);
}

void RMS::on_order_canceled(const Order* order, int canceled) {
  for (auto& rule : rules_) rule->on_order_canceled(order, canceled);
}

void RMS::on_order_rejected(const Order* order, int error_code) {
  for (auto& rule : rules_) rule->on_order_rejected(order, error_code);
}

void RMS::on_order_completed(const Order* order) {
  for (auto& rule : rules_) rule->on_order_completed(order);
}

}  // namespace ft
