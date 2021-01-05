// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "trading_server/risk_management/risk_management.h"

namespace ft {

RiskManagementSystem::RiskManagementSystem() {}

bool RiskManagementSystem::Init(RiskRuleParams* params) {
  for (auto& rule : rules_) {
    if (!rule->Init(params)) return false;
  }

  return true;
}

void RiskManagementSystem::AddRule(std::shared_ptr<RiskRule> rule) { rules_.emplace_back(rule); }

int RiskManagementSystem::CheckOrderRequest(const Order* order) {
  int error_code;

  for (auto& rule : rules_) {
    error_code = rule->CheckOrderRequest(order);
    if (error_code != NO_ERROR) return error_code;
  }

  return NO_ERROR;
}

void RiskManagementSystem::OnOrderSent(const Order* order) {
  for (auto& rule : rules_) rule->OnOrderSent(order);
}

void RiskManagementSystem::OnOrderAccepted(const Order* order) {
  for (auto& rule : rules_) rule->OnOrderAccepted(order);
}

void RiskManagementSystem::OnOrderTraded(const Order* order, const Trade* trade) {
  for (auto& rule : rules_) rule->OnOrderTraded(order, trade);
}

void RiskManagementSystem::OnOrderCanceled(const Order* order, int canceled) {
  for (auto& rule : rules_) rule->OnOrderCanceled(order, canceled);
}

void RiskManagementSystem::OnOrderRejected(const Order* order, int error_code) {
  for (auto& rule : rules_) rule->OnOrderRejected(order, error_code);
}

void RiskManagementSystem::OnOrderCompleted(const Order* order) {
  for (auto& rule : rules_) rule->OnOrderCompleted(order);
}

}  // namespace ft
