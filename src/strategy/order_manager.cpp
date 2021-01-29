// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "strategy/order_manager.h"

namespace ft {

void OrderManager::UpdateOrderStatus(const OrderResponse& rsp) {
  if (rsp.completed) {
    order_map_.erase(rsp.order_id);
    return;
  }

  auto& order_info = order_map_[rsp.order_id];
  if (order_info.ticker_id == 0) {
    order_info.client_order_id = rsp.client_order_id;
    order_info.order_id = rsp.order_id;
    order_info.ticker_id = rsp.ticker_id;
    order_info.direction = rsp.direction;
    order_info.offset = rsp.offset;
    order_info.original_volume = rsp.original_volume;
    order_info.traded_volume = rsp.traded_volume;
  } else {
    order_info.traded_volume = rsp.traded_volume;
  }
}

}  // namespace ft
