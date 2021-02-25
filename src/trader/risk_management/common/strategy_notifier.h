// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_SRC_TRADER_RISK_MANAGEMENT_COMMON_STRATEGY_NOTIFIER_H_
#define FT_SRC_TRADER_RISK_MANAGEMENT_COMMON_STRATEGY_NOTIFIER_H_

#include <ft/utils/redis.h>

#include "trader/risk_management/risk_rule.h"

namespace ft {

class StrategyNotifier : public RiskRule {
 public:
  void OnOrderAccepted(const Order* order);

  void OnOrderTraded(const Order* order, const Trade* trade) override;

  void OnOrderCanceled(const Order* order, int canceled) override;

  void OnOrderRejected(const Order* order, int error_code) override;

  RedisSession rsp_redis_;
};

}  // namespace ft

#endif  // FT_SRC_TRADER_RISK_MANAGEMENT_COMMON_STRATEGY_NOTIFIER_H_
