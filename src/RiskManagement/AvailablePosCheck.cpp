// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "RiskManagement/AvailablePosCheck.h"

#include "Core/Constants.h"

namespace ft {

AvailablePosCheck::AvailablePosCheck(const PositionManager* pos_mgr)
    : pos_mgr_(pos_mgr) {}

int AvailablePosCheck::check_order_req(const OrderReq* order) {
  if (is_offset_close(order->offset)) {
    auto pos = pos_mgr_->find(order->ticker_index);
    int available = 0;
    if (pos) {
      uint32_t d = opp_direction(order->direction);
      auto& detail = d == Direction::BUY ? pos->long_pos : pos->short_pos;
      available = detail.holdings - detail.close_pending;
    }
    if (available < order->volume) {
      spdlog::error(
          "[AvailablePosCheck::check_order_req] Not enough volume to close. "
          "Available: {}, OrderVolume: {}",
          available, order->volume);
      return ERR_POSITION_NOT_ENOUGH;
    }
  }

  return NO_ERROR;
}

}  // namespace ft
