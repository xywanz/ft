// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "cep/order_book/price_level.h"

namespace ft::orderbook {

void PriceLevel::add_order(LimitOrder* order) {
  order_list_.emplace_back(*order);
  total_volume_ += order->volume();
}

void PriceLevel::modify_order(LimitOrder* order) {
  for (auto it = order_list_.begin(); it != order_list_.end(); ++it) {
    if (it->id() == order->id()) {
      total_volume_ += order->volume() - it->volume();
      *it = *order;
      return;
    }
  }
}

void PriceLevel::remove_order(LimitOrder* order) {
  for (auto it = order_list_.begin(); it != order_list_.end(); ++it) {
    if (it->id() == order->id()) {
      total_volume_ -= order->volume();
      order_list_.erase(it);
      return;
    }
  }
}

}  // namespace ft::orderbook
