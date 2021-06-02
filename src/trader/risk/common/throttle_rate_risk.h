// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_SRC_TRADER_RISK_COMMON_THROTTLE_RATE_RISK_H_
#define FT_SRC_TRADER_RISK_COMMON_THROTTLE_RATE_RISK_H_

#include <ctime>
#include <queue>

#include "trader/risk/risk_rule.h"

namespace ft {

class ThrottleRateRisk : public RiskRule {
 public:
  bool Init(RiskRuleParams* params) override;

  ErrorCode CheckOrderRequest(const Order& order) override;

  void OnOrderSent(const Order& order) override;

 private:
  uint64_t GetCurrentTimeMs() {
    timespec ts;
    clock_gettime(CLOCK_MONOTONIC_COARSE, &ts);
    return ts.tv_nsec / 1000000 + ts.tv_sec * 1000;
  }

 private:
  uint64_t order_limit_ = 0;
  uint64_t period_ms_ = 0;
  std::queue<uint64_t> time_queue_;
  uint64_t current_ms_;
};

}  // namespace ft

#endif  // FT_SRC_TRADER_RISK_COMMON_THROTTLE_RATE_RISK_H_
