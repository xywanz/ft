// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_SRC_TRADER_RISK_RMS_H_
#define FT_SRC_TRADER_RISK_RMS_H_

#include <list>
#include <map>
#include <memory>
#include <string>

#include "ft/base/trade_msg.h"
#include "trader/order.h"
#include "trader/risk/risk_rule.h"

namespace ft {

class RiskManagementSystem {
 public:
  RiskManagementSystem();

  bool Init(RiskRuleParams* params);

  bool AddRule(const std::string& risk_rule_name);

  ErrorCode CheckOrderRequest(const Order& order);

  ErrorCode CheckCancelReq(const Order& order);

  void OnOrderSent(const Order& order);

  void OnCancelReqSent(const Order& order);

  void OnOrderAccepted(const Order& order);

  void OnOrderTraded(const Order& order, const OrderTradedRsp& trade);

  void OnOrderCanceled(const Order& order, int canceled);

  void OnOrderRejected(const Order& order, ErrorCode error_code);

  void OnOrderCompleted(const Order& order);

 private:
  std::list<std::shared_ptr<RiskRule>> rules_;
};

}  // namespace ft

#endif  // FT_SRC_TRADER_RISK_RMS_H_