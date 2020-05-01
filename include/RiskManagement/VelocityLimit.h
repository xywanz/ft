// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_INCLUDE_RISKMANAGEMENT_VELOCITYLIMIT_H_
#define FT_INCLUDE_RISKMANAGEMENT_VELOCITYLIMIT_H_

#include <list>
#include <string>
#include <utility>

#include <spdlog/spdlog.h>

#include "Base/Order.h"
#include "RiskManagement/RiskRuleInterface.h"

namespace ft {

inline uint64_t get_current_ms() {
  timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return ts.tv_nsec / 1000000 + ts.tv_sec * 1000;
}

class VelocityLimit : public RiskRuleInterface {
 public:
  VelocityLimit(uint64_t period_ms, uint64_t order_limit, uint64_t volume_limit)
    : period_ms_(period_ms),
      order_limit_(order_limit),
      volume_limit_(volume_limit) {}

  // 返回false则拦截订单
  bool check(const Order* order) {
    if (order_limit_ == 0 && volume_limit_ == 0 || period_ms_ == 0)
      return true;

    uint64_t current_ms = get_current_ms();
    uint64_t lower_bound_ms = current_ms - period_ms_;

    if (order_limit_ > 0) {
      for (auto iter = order_tm_record_.begin(); iter != order_tm_record_.end(); ) {
        if (*iter > lower_bound_ms)
          break;
        --order_count_;
        iter = order_tm_record_.erase(iter);
      }

      if (order_count_ + 1 > order_limit_) {
        spdlog::error("[VelocityLimit::check] Order num reached limit within {} ms. "
                      "Current: {}, Limit: {}",
                      period_ms_, order_count_, order_limit_);
        return false;
      }

      ++order_count_;
      order_tm_record_.emplace_back(current_ms);
    }

    if (volume_limit_ > 0) {
      for (auto iter = volume_tm_record_.begin(); iter != volume_tm_record_.end(); ) {
        if (iter->first > lower_bound_ms)
          break;
        volume_count_ -= iter->second;
        iter = volume_tm_record_.erase(iter);
      }

      if (volume_count_ + order->volume > volume_limit_) {
        spdlog::error("[VelocityLimit::check] Volume reach limit within {} ms. "
                      "This Order: {}, Current: {}, Limit: {}",
                      period_ms_, order->volume, volume_count_, volume_limit_);
        return false;
      }

      volume_count_ += order->volume;
      volume_tm_record_.emplace_back(std::pair{current_ms, order->volume});
    }
  }

 private:
  uint64_t order_limit_ = 0;
  uint64_t volume_limit_ = 0;
  uint64_t period_ms_ = 0;

  uint64_t order_count_ = 0;
  uint64_t volume_count_ = 0;
  std::list<uint64_t> order_tm_record_;
  std::list<std::pair<uint64_t, int64_t>> volume_tm_record_;
};

}  // namespace ft

#endif  // FT_INCLUDE_RISKMANAGEMENT_VELOCITYLIMIT_H_
