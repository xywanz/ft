// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_SRC_RISK_MANAGEMENT_COMMON_THROTTLE_RATE_LIMIT_H_
#define FT_SRC_RISK_MANAGEMENT_COMMON_THROTTLE_RATE_LIMIT_H_

#include <spdlog/spdlog.h>

#include <list>
#include <map>
#include <string>
#include <tuple>
#include <utility>

#include "risk_management/risk_rule_interface.h"

namespace ft {

inline uint64_t get_current_ms() {
  timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return ts.tv_nsec / 1000000 + ts.tv_sec * 1000;
}

class ThrottleRateLimit : public RiskRuleInterface {
 public:
  bool init(const Config& config, Account* account, Portfolio* portfolio,
            OrderMap* order_map, const MdSnapshot* md_snapshot) override;

  int check_order_req(const Order* order) override;

  void on_order_rejected(const Order* order, int error_code) override;

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

#endif  // FT_SRC_RISK_MANAGEMENT_THROTTLE_RATE_LIMIT_H_
