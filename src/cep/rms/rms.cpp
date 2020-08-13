// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "cep/rms/rms.h"

namespace ft {

RMS::RMS() {}

bool RMS::init(RiskRuleParams* params) {
  for (auto& rule : rules_) {
    if (!rule->init(params)) return false;
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
