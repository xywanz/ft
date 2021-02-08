// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_SRC_TRADING_SERVER_COMMON_ORDER_H_
#define FT_SRC_TRADING_SERVER_COMMON_ORDER_H_

#include <map>
#include <string>

#include "protocol/data_types.h"

namespace ft {

// OMS发给Gateway的订单请求

struct Order {
  OrderRequest req;

  // 这个ID是策略发单的时候提供的，使策略能定位其订单，类似于备注
  uint32_t client_order_id;

  bool accepted = false;
  int traded_volume = 0;
  int canceled_volume = 0;
  OrderStatus status;
  uint64_t privdata;
  uint64_t insert_time;
  std::string strategy_id;
};

}  // namespace ft

#endif  // FT_SRC_TRADING_SERVER_COMMON_ORDER_H_
