// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_SRC_RISK_MANAGEMENT_ETF_ARBITRAGE_ARBITRAGE_MANAGER_H_
#define FT_SRC_RISK_MANAGEMENT_ETF_ARBITRAGE_ARBITRAGE_MANAGER_H_

#include <map>

#include "risk_management/risk_rule_interface.h"

namespace ft {

class ArbitrageManager : public RiskRuleInterface {
 public:
  bool init(const Config& config, Account* account, Portfolio* portfolio,
            std::map<uint64_t, Order>* order_map,
            const MdSnapshot* md_snapshot) override;

  int check_order_req(const Order* order) override;

  void on_order_sent(const Order* order) {}

  void on_order_accepted(const Order* order) {}

  void on_order_traded(const Order* order, const OrderTradedRsp* trade) {}

  void on_order_canceled(const Order* order, int canceled) {}

  void on_order_completed(const Order* order) {}

  void on_order_rejected(const Order* order, int error_code) {}

 private:
  Account* account_;
  Portfolio* portfolio_;
  std::map<uint64_t, Order>* order_map_;
  const MdSnapshot* md_snapshot_;
};

}  // namespace ft

#endif  // FT_SRC_RISK_MANAGEMENT_ETF_ARBITRAGE_ARBITRAGE_MANAGER_H_
