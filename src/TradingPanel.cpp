// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "TradingPanel.h"

namespace ft {

TradingPanel::TradingPanel() : portfolio_("127.0.0.1", 6379) {}

void TradingPanel::update_order(const Order* rtn_order) {
  if (orders_.find(rtn_order->order_id) == orders_.end()) {
    spdlog::warn(
        "[TradingPanel::update_order] Order not found."
        " Order: <OrderID: {}, Direction: {}, "
        "Offset: {}, OrderType: {}, Traded: {}, Total: {}, Price: {:.2f}, "
        "Status: {}>",
        rtn_order->order_id, to_string(rtn_order->direction),
        to_string(rtn_order->offset), to_string(rtn_order->type),
        rtn_order->volume_traded, rtn_order->volume, rtn_order->price,
        to_string(rtn_order->status));
    return;
  }

  switch (rtn_order->status) {
    case OrderStatus::REJECTED:
    case OrderStatus::CANCELED:
      portfolio_.update_pending(
          rtn_order->ticker_index, rtn_order->direction, rtn_order->offset,
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
      break;
  }

  spdlog::info(
      "[TradingPanel::update_order] Order status's updated."
      " Order: <OrderID: {}, Direction: {}, "
      "Offset: {}, OrderType: {}, Traded: {}, Total: {}, Price: {:.2f}, "
      "Status: {}>",
      rtn_order->order_id, to_string(rtn_order->direction),
      to_string(rtn_order->offset), to_string(rtn_order->type),
      rtn_order->volume_traded, rtn_order->volume, rtn_order->price,
      to_string(rtn_order->status));
}

}  // namespace ft
