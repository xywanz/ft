// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "risk_management/etf_arbitrage/arbitrage_manager.h"

#include "risk_management/etf_arbitrage/etf_table.h"

namespace ft {

bool ArbitrageManager::init(const Config& config, Account* account,
                            Portfolio* portfolio,
                            std::map<uint64_t, Order>* order_map) {
  return EtfTable::init("../config/etf_list.csv",
                        "../config/etf_components.csv");
}

int ArbitrageManager::check_order_req(const Order* order) { return NO_ERROR; }

}  // namespace ft
