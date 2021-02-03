// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_SRC_TRADING_SERVER_RISK_MANAGEMENT_RISK_RULE_H_
#define FT_SRC_TRADING_SERVER_RISK_MANAGEMENT_RISK_RULE_H_

#include <map>
#include <string>

#include "trading_server/datastruct/account.h"
#include "trading_server/datastruct/config.h"
#include "trading_server/datastruct/error_code.h"
#include "trading_server/datastruct/md_snapshot.h"
#include "trading_server/datastruct/order.h"
#include "trading_server/order_management/base_oms.h"
#include "trading_server/risk_management/types.h"
#include "utils/portfolio.h"

namespace ft {

struct RiskRuleParams {
  const Config* config;
  const MarketDataSnashot* md_snapshot;
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

#endif  // FT_SRC_TRADING_SERVER_RISK_MANAGEMENT_RISK_RULE_H_
