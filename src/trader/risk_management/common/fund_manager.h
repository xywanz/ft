// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_SRC_TRADER_RISK_MANAGEMENT_COMMON_FUND_MANAGER_H_
#define FT_SRC_TRADER_RISK_MANAGEMENT_COMMON_FUND_MANAGER_H_

#include <map>

#include "ft/base/trade_msg.h"
#include "trader/risk_management/risk_rule.h"

namespace ft {

class FundManager : public RiskRule {
 public:
  bool Init(RiskRuleParams* params) override;

  int CheckOrderRequest(const Order* order) override;

  void OnOrderSent(const Order* order) override;

  void OnOrderTraded(const Order* order, const Trade* trade) override;

  void OnOrderCanceled(const Order* order, int canceled) override;

  void OnOrderRejected(const Order* order, int error_code) override;

 private:
  Account* account_{nullptr};
};

}  // namespace ft

#endif  // FT_SRC_TRADER_RISK_MANAGEMENT_COMMON_FUND_MANAGER_H_
