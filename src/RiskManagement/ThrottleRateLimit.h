// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_SRC_RISKMANAGEMENT_THROTTLERATELIMIT_H_
#define FT_SRC_RISKMANAGEMENT_THROTTLERATELIMIT_H_

#include <spdlog/spdlog.h>

#include <list>
#include <string>
#include <tuple>
#include <utility>

#include "RiskManagement/RiskRuleInterface.h"

namespace ft {

inline uint64_t get_current_ms() {
  timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return ts.tv_nsec / 1000000 + ts.tv_sec * 1000;
}

class ThrottleRateLimit : public RiskRuleInterface {
 public:
  ThrottleRateLimit(uint64_t period_ms, uint64_t order_limit,
                    uint64_t volume_limit);

  // 返回false则拦截订单
  bool check_order_req(const OrderReq* order) override;

  void on_order_completed(uint64_t engine_order_id, int error_code) override;

 private:
  uint64_t order_limit_ = 0;
  uint64_t volume_limit_ = 0;
  uint64_t period_ms_ = 0;

  uint64_t volume_count_ = 0;

  // (time_ms, engine_order_id)
  std::list<std::tuple<uint64_t, uint64_t>> order_tm_record_;

  // (time_ms, volume, engine_order_id)
  std::list<std::tuple<uint64_t, int, uint64_t>> volume_tm_record_;
};

}  // namespace ft

#endif  // FT_SRC_RISKMANAGEMENT_THROTTLERATELIMIT_H_
