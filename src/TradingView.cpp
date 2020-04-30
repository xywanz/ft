// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "TradingManagement/TradingView.h"

namespace ft {

void TradingView::update_order(const Order* rtn_order) {
  if (orders_.find(rtn_order->order_id) == orders_.end()) {
    spdlog::warn("[TradingView::update_order] Order not found. Ticker: {}, "
                 "Order ID: {}, Direction: {}, Offset: {}, Volume: {}, Status: {}",
                 rtn_order->ticker, rtn_order->order_id, to_string(rtn_order->direction),
                 to_string(rtn_order->offset), rtn_order->volume, to_string(rtn_order->status));
    return;
  }

  switch (rtn_order->status) {
  case OrderStatus::SUBMITTING:
    break;
  case OrderStatus::REJECTED:
  case OrderStatus::CANCELED:
    handle_canceled(rtn_order);
    break;
  case OrderStatus::NO_TRADED:
    handle_submitted(rtn_order);
    break;
  case OrderStatus::PART_TRADED:
    handle_part_traded(rtn_order);
    break;
  case OrderStatus::ALL_TRADED:
    handle_all_traded(rtn_order);
    break;
  case OrderStatus::CANCEL_REJECTED:
    handle_cancel_rejected(rtn_order);
    break;
  default:
    assert(false);
  }

  spdlog::info("[TradingView::update_order] Ticker: {}, Order ID: {}, Direction: {}, "
               "Offset: {}, Traded: {}, Origin Volume: {}, Status: {}",
               rtn_order->ticker, rtn_order->order_id, to_string(rtn_order->direction),
               to_string(rtn_order->offset), rtn_order->volume_traded, rtn_order->volume,
               to_string(rtn_order->status));
}

void TradingView::handle_canceled(const Order* rtn_order) {
  orders_.erase(rtn_order->order_id);

  auto left_vol = rtn_order->volume - rtn_order->volume_traded;
  portfolio_.update_pending(rtn_order->ticker, rtn_order->direction, rtn_order->offset, -left_vol);
}

void TradingView::handle_submitted(const Order* rtn_order) {
  auto& order = orders_[rtn_order->order_id];
  order.status = OrderStatus::NO_TRADED;
}

void TradingView::handle_part_traded(const Order* rtn_order) {
  auto& order = orders_[rtn_order->order_id];
  if (rtn_order->volume_traded > order.volume_traded)
    order.volume_traded = rtn_order->volume_traded;
  order.status = OrderStatus::PART_TRADED;
}

void TradingView::handle_all_traded(const Order* rtn_order) {
  orders_.erase(rtn_order->order_id);
}

void TradingView::handle_cancel_rejected(const Order* rtn_order) {
    auto& order = orders_[rtn_order->order_id];
}

}  // namespace ft
