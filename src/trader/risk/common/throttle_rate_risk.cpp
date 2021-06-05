// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "trader/risk/common/throttle_rate_risk.h"

#include "ft/base/log.h"
#include "ft/utils/protocol_utils.h"

namespace ft {

bool ThrottleRateRisk::Init(RiskRuleParams* params) {
  auto& risk_conf = params->config->risk_conf_list[GetId()];
  for (auto& [opt, val] : risk_conf.options) {
    if (opt == "order_limit") {
      order_limit_ = std::stoull(val);
    } else if (opt == "period_ms") {
      period_ms_ = std::stoull(val);
    } else {
      LOG_ERROR("unknown opt {}:{}", opt, val);
      return false;
    }
  }
  LOG_INFO("throttle rate risk inited");
  return true;
}

ErrorCode ThrottleRateRisk::CheckOrderRequest(const Order& order) {
  if (order_limit_ == 0 || period_ms_ == 0) {
    return ErrorCode::kNoError;
  }

  current_ms_ = GetCurrentTimeMs();
  uint64_t lower_bound_ms = current_ms_ - period_ms_;

  while (!time_queue_.empty()) {
    auto oldest = time_queue_.front();
    if (oldest > lower_bound_ms) {
      break;
    }
    time_queue_.pop();
  }

  if (time_queue_.size() >= order_limit_) {
    LOG_ERROR("Order num reached limit within {} ms. Current: {}, Limit: {}", period_ms_,
              time_queue_.size(), order_limit_);
    return ErrorCode::kExceedThrottleRateRisk;
  }

  return ErrorCode::kNoError;
}

void ThrottleRateRisk::OnOrderSent(const Order& order) {
  if (order_limit_ > 0 && period_ms_ > 0) {
    time_queue_.push(current_ms_);
  }
}

REGISTER_RISK_RULE("ft.risk.throttle_rate", ThrottleRateRisk);

}  // namespace ft
