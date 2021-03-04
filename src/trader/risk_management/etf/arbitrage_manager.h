// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_SRC_TRADER_RISK_MANAGEMENT_ETF_ARBITRAGE_MANAGER_H_
#define FT_SRC_TRADER_RISK_MANAGEMENT_ETF_ARBITRAGE_MANAGER_H_

#include <map>

#include "trader/risk_management/risk_rule.h"

namespace ft {

class ArbitrageManager : public RiskRule {
 public:
  bool Init(RiskRuleParams* params) override;

  int CheckOrderRequest(const Order* order) override;

  void OnOrderSent(const Order* order) {}

  void OnOrderAccepted(const Order* order) {}

  void OnOrderTraded(const Order* order, const Trade* trade) {}

  void OnOrderCanceled(const Order* order, int canceled) {}

  void OnOrderCompleted(const Order* order) {}

  void OnOrderRejected(const Order* order, int error_code) {}

 private:
  Account* account_;
  PositionCalculator* pos_calculator_;
  OrderMap* order_map_;
};

}  // namespace ft

#endif  // FT_SRC_TRADER_RISK_MANAGEMENT_ETF_ARBITRAGE_MANAGER_H_
