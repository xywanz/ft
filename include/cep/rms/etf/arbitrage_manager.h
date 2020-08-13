// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_SRC_RISK_MANAGEMENT_ETF_ARBITRAGE_MANAGER_H_
#define FT_SRC_RISK_MANAGEMENT_ETF_ARBITRAGE_MANAGER_H_

#include <map>

#include "cep/rms/risk_rule.h"

namespace ft {

class ArbitrageManager : public RiskRule {
 public:
  bool init(RiskRuleParams* params) override;

  int check_order_req(const Order* order) override;

  void on_order_sent(const Order* order) {}

  void on_order_accepted(const Order* order) {}

  void on_order_traded(const Order* order, const Trade* trade) {}

  void on_order_canceled(const Order* order, int canceled) {}

  void on_order_completed(const Order* order) {}

  void on_order_rejected(const Order* order, int error_code) {}

 private:
  Account* account_;
  Portfolio* portfolio_;
  OrderMap* order_map_;
  const MdSnapshot* md_snapshot_;
};

}  // namespace ft

#endif  // FT_SRC_RISK_MANAGEMENT_ETF_ARBITRAGE_MANAGER_H_
