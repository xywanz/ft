// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "trader/risk/common/throttle_rate_risk.h"

#include "ft/base/log.h"
#include "ft/utils/protocol_utils.h"

namespace ft {

bool ThrottleRateRisk::Init(RiskRuleParams* params) {
  period_ms_ = params->config->throttle_rate_limit_period_ms;
  order_limit_ = params->config->throttle_rate_order_limit;
  volume_limit_ = params->config->throttle_rate_volume_limit;

  return true;
}

ErrorCode ThrottleRateRisk::CheckOrderRequest(const Order& order) {
  auto& req = order.req;
  if ((order_limit_ == 0 && volume_limit_ == 0) || period_ms_ == 0) {
    return ErrorCode::kNoError;
  }

  current_ms_ = GetCurrentTimeMs();
  uint64_t lower_bound_ms = current_ms_ - period_ms_;

  if (order_limit_ > 0) {
    for (auto iter = order_tm_record_.begin(); iter != order_tm_record_.end();) {
      if (std::get<0>(*iter) > lower_bound_ms) {
        break;
      }
      iter = order_tm_record_.erase(iter);
    }

    if (order_tm_record_.size() >= order_limit_) {
      LOG_ERROR(
          "[ThrottleRateRisk::check] Order num reached limit within {} ms. "
          "Current: {}, Limit: {}",
          period_ms_, order_tm_record_.size(), order_limit_);
      return ErrorCode::kExceedThrottleRateRisk;
    }
  }

  if (volume_limit_ > 0) {
    for (auto iter = volume_tm_record_.begin(); iter != volume_tm_record_.end();) {
      if (std::get<0>(*iter) > lower_bound_ms) {
        break;
      }
      volume_count_ -= std::get<1>(*iter);
      iter = volume_tm_record_.erase(iter);
    }

    if (volume_count_ + req.volume > volume_limit_) {
      LOG_ERROR(
          "[ThrottleRateRisk::check] Volume reach limit within {} ms. "
          "This Order: {}, Current: {}, Limit: {}",
          period_ms_, req.volume, volume_count_, volume_limit_);
      return ErrorCode::kExceedThrottleRateRisk;
    }
  }

  return ErrorCode::kNoError;
}

void ThrottleRateRisk::OnOrderSent(const Order& order) {
  uint64_t lower_bound_ms = current_ms_ - period_ms_;

  if (order_limit_ > 0) {
    order_tm_record_.emplace_back(std::make_tuple(current_ms_, order.req.order_id));
  }

  if (volume_limit_ > 0) {
    volume_count_ += order.req.volume;
    volume_tm_record_.emplace_back(
        std::make_tuple(current_ms_, order.req.volume, order.req.order_id));
  }
}

REGISTER_RISK_RULE("throttle_rate_risk", ThrottleRateRisk);

}  // namespace ft
