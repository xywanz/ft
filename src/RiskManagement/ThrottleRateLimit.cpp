// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "RiskManagement/ThrottleRateLimit.h"

namespace ft {

ThrottleRateLimit::ThrottleRateLimit(uint64_t period_ms, uint64_t order_limit,
                                     uint64_t volume_limit)
    : order_limit_(order_limit),
      volume_limit_(volume_limit),
      period_ms_(period_ms) {}

bool ThrottleRateLimit::check_order_req(const OrderReq* order) {
  if ((order_limit_ == 0 && volume_limit_ == 0) || period_ms_ == 0) return true;

  uint64_t current_ms = get_current_ms();
  uint64_t lower_bound_ms = current_ms - period_ms_;

  if (order_limit_ > 0) {
    for (auto iter = order_tm_record_.begin();
         iter != order_tm_record_.end();) {
      if (*iter > lower_bound_ms) break;
      --order_count_;
      iter = order_tm_record_.erase(iter);
    }

    if (order_count_ + 1 > order_limit_) {
      spdlog::error(
          "[ThrottleRateLimit::check] Order num reached limit within {} ms. "
          "Current: {}, Limit: {}",
          period_ms_, order_count_, order_limit_);
      return false;
    }

    ++order_count_;
    order_tm_record_.emplace_back(current_ms);
  }

  if (volume_limit_ > 0) {
    for (auto iter = volume_tm_record_.begin();
         iter != volume_tm_record_.end();) {
      if (iter->first > lower_bound_ms) break;
      volume_count_ -= iter->second;
      iter = volume_tm_record_.erase(iter);
    }

    if (volume_count_ + order->volume > volume_limit_) {
      spdlog::error(
          "[ThrottleRateLimit::check] Volume reach limit within {} ms. "
          "This Order: {}, Current: {}, Limit: {}",
          period_ms_, order->volume, volume_count_, volume_limit_);
      return false;
    }

    volume_count_ += order->volume;
    volume_tm_record_.emplace_back(std::pair{current_ms, order->volume});
  }

  return true;
}

}  // namespace ft
