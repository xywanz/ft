// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "TradingPanel.h"

namespace ft {

void TradingPanel::update_order(const Order* rtn_order) {
  if (orders_.find(rtn_order->order_id) == orders_.end()) {
    PRINT_ORDER(spdlog::warn, "", rtn_order, "Order not found.");
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

  PRINT_ORDER(spdlog::info, "", rtn_order, "Order status's updated.");
}

}  // namespace ft
