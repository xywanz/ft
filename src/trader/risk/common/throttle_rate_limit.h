// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_SRC_TRADER_RISK_COMMON_THROTTLE_RATE_LIMIT_H_
#define FT_SRC_TRADER_RISK_COMMON_THROTTLE_RATE_LIMIT_H_

#include <list>
#include <map>
#include <string>
#include <tuple>
#include <utility>

#include "spdlog/spdlog.h"
#include "trader/risk/risk_rule.h"

namespace ft {

class ThrottleRateLimit : public RiskRule {
 public:
  bool Init(RiskRuleParams* params) override;

  ErrorCode CheckOrderRequest(const Order& order) override;

  void OnOrderSent(const Order& order) override;

 private:
  uint64_t GetCurrentTimeMs() {
    timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_nsec / 1000000 + ts.tv_sec * 1000;
  }

 private:
  uint64_t order_limit_ = 0;
  uint64_t volume_limit_ = 0;
  uint64_t period_ms_ = 0;

  uint64_t volume_count_ = 0;

  // (time_ms, oms_order_id)
  std::list<std::tuple<uint64_t, uint64_t>> order_tm_record_;

  // (time_ms, volume, oms_order_id)
  std::list<std::tuple<uint64_t, int, uint64_t>> volume_tm_record_;

  uint64_t current_ms_;
};

}  // namespace ft

#endif  // FT_SRC_TRADER_RISK_COMMON_THROTTLE_RATE_LIMIT_H_
