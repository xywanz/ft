// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_SRC_TRADER_RISK_MANAGEMENT_RISK_MANAGEMENT_H_
#define FT_SRC_TRADER_RISK_MANAGEMENT_RISK_MANAGEMENT_H_

#include <ft/base/trade_msg.h>

#include <list>
#include <map>
#include <memory>
#include <string>

#include "trader/order.h"
#include "trader/risk_management/risk_rule.h"

namespace ft {

class RiskManagementSystem {
 public:
  RiskManagementSystem();

  bool Init(RiskRuleParams* params);

  void AddRule(std::shared_ptr<RiskRule> rule);

  int CheckOrderRequest(const Order* order);

  void OnOrderSent(const Order* order);

  void OnOrderAccepted(const Order* order);

  void OnOrderTraded(const Order* order, const Trade* trade);

  void OnOrderCanceled(const Order* order, int canceled);

  void OnOrderRejected(const Order* order, int error_code);

  void OnOrderCompleted(const Order* order);

 private:
  std::list<std::shared_ptr<RiskRule>> rules_;
};

}  // namespace ft

#endif  // FT_SRC_TRADER_RISK_MANAGEMENT_RISK_MANAGEMENT_H_