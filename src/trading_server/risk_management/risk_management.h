// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_SRC_TRADING_SERVER_RISK_MANAGEMENT_RISK_MANAGEMENT_H_
#define FT_SRC_TRADING_SERVER_RISK_MANAGEMENT_RISK_MANAGEMENT_H_

#include <list>
#include <map>
#include <memory>
#include <string>

#include "trading_server/datastruct/all.h"
#include "trading_server/order_management/base_oms.h"
#include "trading_server/order_management/portfolio.h"
#include "trading_server/risk_management/risk_rule.h"
#include "trading_server/risk_management/types.h"

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

#endif  // FT_SRC_TRADING_SERVER_RISK_MANAGEMENT_RISK_MANAGEMENT_H_