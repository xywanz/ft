// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "TradingInfo/TradingPanel.h"

namespace ft {

void TradingPanel::update_order(const Order* rtn_order) {
  if (orders_.find(rtn_order->order_id) == orders_.end()) {
    spdlog::warn("[TradingPanel::update_order] Order not found. Ticker: {}, "
                 "Order ID: {}, Direction: {}, Offset: {}, Volume: {}, Status: {}",
                 rtn_order->ticker, rtn_order->order_id, to_string(rtn_order->direction),
                 to_string(rtn_order->offset), rtn_order->volume, to_string(rtn_order->status));
    return;
  }

  switch (rtn_order->status) {
  case OrderStatus::REJECTED:
  case OrderStatus::CANCELED:
    portfolio_.update_pending(rtn_order->ticker, rtn_order->direction, rtn_order->offset,
                              -(rtn_order->volume - rtn_order->volume_traded));
    orders_.erase(rtn_order->order_id);
    break;
  case OrderStatus::NO_TRADED:
  case OrderStatus::PART_TRADED:
    orders_[rtn_order->order_id] = *rtn_order;
    break;
  case OrderStatus::ALL_TRADED:
    orders_.erase(rtn_order->order_id);
    break;
  default:
    assert(false);
  }

  spdlog::info("[TradingPanel::update_order] Ticker: {}, Order ID: {}, Direction: {}, "
               "Offset: {}, Traded: {}, Origin Volume: {}, Status: {}",
               rtn_order->ticker, rtn_order->order_id, to_string(rtn_order->direction),
               to_string(rtn_order->offset), rtn_order->volume_traded, rtn_order->volume,
               to_string(rtn_order->status));
}

}  // namespace ft
