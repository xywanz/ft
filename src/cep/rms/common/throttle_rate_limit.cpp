// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "cep/rms/common/throttle_rate_limit.h"

#include "cep/data/error_code.h"

namespace ft {

bool ThrottleRateLimit::init(RiskRuleParams* params) {
  period_ms_ = params->config->throttle_rate_limit_period_ms;
  order_limit_ = params->config->throttle_rate_order_limit;
  volume_limit_ = params->config->throttle_rate_volume_limit;

  return true;
}

int ThrottleRateLimit::check_order_req(const Order* order) {
  auto* req = &order->req;
  if ((order_limit_ == 0 && volume_limit_ == 0) || period_ms_ == 0)
    return NO_ERROR;

  uint64_t current_ms = get_current_ms();
  uint64_t lower_bound_ms = current_ms - period_ms_;

  if (order_limit_ > 0) {
    for (auto iter = order_tm_record_.begin();
         iter != order_tm_record_.end();) {
      if (std::get<0>(*iter) > lower_bound_ms) break;
      iter = order_tm_record_.erase(iter);
    }

    if (order_tm_record_.size() >= order_limit_) {
      spdlog::error(
          "[ThrottleRateLimit::check] Order num reached limit within {} ms. "
          "Current: {}, Limit: {}",
          period_ms_, order_tm_record_.size(), order_limit_);
      return ERR_THROTTLE_RATE_LIMIT;
    }

    order_tm_record_.emplace_back(
        std::make_tuple(current_ms, order->req.order_id));
  }

  if (volume_limit_ > 0) {
    for (auto iter = volume_tm_record_.begin();
         iter != volume_tm_record_.end();) {
      if (std::get<0>(*iter) > lower_bound_ms) break;
      volume_count_ -= std::get<1>(*iter);
      iter = volume_tm_record_.erase(iter);
    }

    if (volume_count_ + req->volume > volume_limit_) {
      spdlog::error(
          "[ThrottleRateLimit::check] Volume reach limit within {} ms. "
          "This Order: {}, Current: {}, Limit: {}",
          period_ms_, req->volume, volume_count_, volume_limit_);
      return ERR_THROTTLE_RATE_LIMIT;
    }

    volume_count_ += req->volume;
    volume_tm_record_.emplace_back(
        std::make_tuple(current_ms, req->volume, order->req.order_id));
  }

  return NO_ERROR;
}

void ThrottleRateLimit::on_order_rejected(const Order* order, int error_code) {
  if (error_code <= ERR_SEND_FAILED) {
    if (!volume_tm_record_.empty() &&
        std::get<2>(volume_tm_record_.back()) == order->req.order_id) {
      volume_count_ -= std::get<1>(volume_tm_record_.back());
      volume_tm_record_.pop_back();
    }

    if (!order_tm_record_.empty() &&
        std::get<1>(order_tm_record_.back()) == order->req.order_id) {
      order_tm_record_.pop_back();
    }
  }
}

}  // namespace ft
