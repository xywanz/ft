// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "ft/component/order_book/price_level.h"

namespace ft::orderbook {

void PriceLevel::AddOrder(LimitOrder* order) {
  order_list_.emplace_back(*order);
  total_volume_ += order->volume();
}

void PriceLevel::ModifyOrder(LimitOrder* order) {
  for (auto it = order_list_.begin(); it != order_list_.end(); ++it) {
    if (it->id() == order->id()) {
      total_volume_ += order->volume() - it->volume();
      *it = *order;
      return;
    }
  }
}

void PriceLevel::RemoveOrder(LimitOrder* order) {
  for (auto it = order_list_.begin(); it != order_list_.end(); ++it) {
    if (it->id() == order->id()) {
      total_volume_ -= order->volume();
      order_list_.erase(it);
      return;
    }
  }
}

}  // namespace ft::orderbook
