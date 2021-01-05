// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "trading_server/risk_management/etf/arbitrage_manager.h"

#include "trading_server/risk_management/etf/etf_table.h"

namespace ft {

bool ArbitrageManager::Init(RiskRuleParams* params) {
  if (params->config->arg0.empty() || params->config->arg1.empty()) return false;

  account_ = params->account;
  portfolio_ = params->portfolio;
  order_map_ = params->order_map;
  md_snapshot_ = params->md_snapshot;

  return EtfTable::Init(params->config->arg0, params->config->arg1);
}

int ArbitrageManager::CheckOrderRequest(const Order* order) { return NO_ERROR; }

}  // namespace ft
