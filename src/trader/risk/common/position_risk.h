// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_SRC_TRADER_RISK_COMMON_POSITION_RISK_H_
#define FT_SRC_TRADER_RISK_COMMON_POSITION_RISK_H_

#include <map>

#include "trader/risk/risk_rule.h"

namespace ft {

class PositionRisk : public RiskRule {
 public:
  bool Init(RiskRuleParams* params) override;

  int CheckOrderRequest(const Order& order) override;

  void OnOrderSent(const Order& order) override;

  void OnOrderTraded(const Order& order, const Trade& trade) override;

  void OnOrderCanceled(const Order& order, int canceled) override;

  void OnOrderRejected(const Order& order, int error_code) override;

 private:
  PositionManager* pos_manager_;
};

};  // namespace ft

#endif  // FT_SRC_TRADER_RISK_COMMON_POSITION_RISK_H_
