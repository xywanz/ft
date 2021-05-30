// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_SRC_TRADER_RISK_COMMON_FUND_RISK_H_
#define FT_SRC_TRADER_RISK_COMMON_FUND_RISK_H_

#include <map>

#include "ft/base/trade_msg.h"
#include "trader/risk/risk_rule.h"

namespace ft {

class FundRisk : public RiskRule {
 public:
  bool Init(RiskRuleParams* params) override;

  ErrorCode CheckOrderRequest(const Order& order) override;

  void OnOrderSent(const Order& order) override;

  void OnOrderTraded(const Order& order, const OrderTradedRsp& trade) override;

  void OnOrderCanceled(const Order& order, int canceled) override;

  void OnOrderRejected(const Order& order, ErrorCode error_code) override;

 private:
  Account* account_{nullptr};
};

}  // namespace ft

#endif  // FT_SRC_TRADER_RISK_COMMON_FUND_RISK_H_
