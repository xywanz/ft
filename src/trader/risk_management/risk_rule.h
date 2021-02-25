// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_SRC_TRADER_RISK_MANAGEMENT_RISK_RULE_H_
#define FT_SRC_TRADER_RISK_MANAGEMENT_RISK_RULE_H_

#include <ft/base/config.h>
#include <ft/base/error_code.h>
#include <ft/base/trade_msg.h>
#include <ft/trader/base_oms.h>
#include <ft/utils/portfolio.h>
#include <ft/utils/protocol_utils.h>

#include <map>
#include <string>
#include <unordered_map>

#include "trader/order.h"

namespace ft {

using OrderMap = std::unordered_map<uint64_t, Order>;

struct RiskRuleParams {
  const Config* config;
  Account* account;
  Portfolio* portfolio;
  OrderMap* order_map;
};

class RiskRule {
 public:
  virtual ~RiskRule() {}

  virtual bool Init(RiskRuleParams* params) { return true; }

  virtual int CheckOrderRequest(const Order* order) { return NO_ERROR; }

  virtual void OnOrderSent(const Order* order) {}

  virtual void OnOrderAccepted(const Order* order) {}

  virtual void OnOrderTraded(const Order* order, const Trade* trade) {}

  virtual void OnOrderCanceled(const Order* order, int canceled) {}

  virtual void OnOrderCompleted(const Order* order) {}

  virtual void OnOrderRejected(const Order* order, int error_code) {}
};

}  // namespace ft

#endif  // FT_SRC_TRADER_RISK_MANAGEMENT_RISK_RULE_H_
