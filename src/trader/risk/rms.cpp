// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "trader/risk/rms.h"

namespace ft {

RiskManagementSystem::RiskManagementSystem() {}

bool RiskManagementSystem::Init(RiskRuleParams* params) {
  for (auto& rule : rules_) {
    if (!rule->Init(params)) return false;
  }

  return true;
}

bool RiskManagementSystem::AddRule(const std::string& risk_rule_name) {
  auto rule = CreateRiskRule(risk_rule_name);
  if (!rule) {
    return false;
  }
  rule->SetId(static_cast<uint32_t>(rules_.size()));
  rules_.emplace_back(rule);
  return true;
}

ErrorCode RiskManagementSystem::CheckOrderRequest(const Order& order) {
  ErrorCode error_code;

  for (auto& rule : rules_) {
    error_code = rule->CheckOrderRequest(order);
    if (error_code != ErrorCode::kNoError) {
      return error_code;
    }
  }

  return ErrorCode::kNoError;
}

ErrorCode RiskManagementSystem::CheckCancelReq(const Order& order) {
  ErrorCode error_code;

  for (auto& rule : rules_) {
    error_code = rule->CheckCancelReq(order);
    if (error_code != ErrorCode::kNoError) {
      return error_code;
    }
  }

  return ErrorCode::kNoError;
}

void RiskManagementSystem::OnOrderSent(const Order& order) {
  for (auto& rule : rules_) {
    rule->OnOrderSent(order);
  }
}

void RiskManagementSystem::OnCancelReqSent(const Order& order) {
  for (auto& rule : rules_) {
    rule->OnCancelReqSent(order);
  }
}

void RiskManagementSystem::OnOrderAccepted(const Order& order) {
  for (auto& rule : rules_) {
    rule->OnOrderAccepted(order);
  }
}

void RiskManagementSystem::OnOrderTraded(const Order& order, const OrderTradedRsp& trade) {
  for (auto& rule : rules_) {
    rule->OnOrderTraded(order, trade);
  }
}

void RiskManagementSystem::OnOrderCanceled(const Order& order, int canceled) {
  for (auto& rule : rules_) {
    rule->OnOrderCanceled(order, canceled);
  }
}

void RiskManagementSystem::OnOrderRejected(const Order& order, ErrorCode error_code) {
  for (auto& rule : rules_) {
    rule->OnOrderRejected(order, error_code);
  }
}

void RiskManagementSystem::OnOrderCompleted(const Order& order) {
  for (auto& rule : rules_) {
    rule->OnOrderCompleted(order);
  }
}

}  // namespace ft
