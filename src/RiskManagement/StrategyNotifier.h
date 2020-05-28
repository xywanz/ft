// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_SRC_RISKMANAGEMENT_STRATEGYNOTIFIER_H_
#define FT_SRC_RISKMANAGEMENT_STRATEGYNOTIFIER_H_

#include "IPC/redis.h"
#include "RiskManagement/RiskRuleInterface.h"

namespace ft {

class StrategyNotifier : public RiskRuleInterface {
 public:
  void on_order_accepted(const Order* order);

  void on_order_traded(const Order* order, int this_traded,
                       double traded_price) override;

  void on_order_canceled(const Order* order, int canceled) override;

  void on_order_rejected(const Order* order, int error_code) override;

  RedisSession rsp_redis_;
};

}  // namespace ft

#endif  // FT_SRC_RISKMANAGEMENT_STRATEGYNOTIFIER_H_
