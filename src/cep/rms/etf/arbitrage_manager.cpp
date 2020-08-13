// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "cep/rms/etf/arbitrage_manager.h"

#include "cep/rms/etf/etf_table.h"

namespace ft {

bool ArbitrageManager::init(RiskRuleParams* params) {
  if (params->config->arg0.empty() || params->config->arg1.empty())
    return false;

  account_ = params->account;
  portfolio_ = params->portfolio;
  order_map_ = params->order_map;
  md_snapshot_ = params->md_snapshot;

  return EtfTable::init(params->config->arg0, params->config->arg1);
}

int ArbitrageManager::check_order_req(const Order* order) { return NO_ERROR; }

}  // namespace ft
