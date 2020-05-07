// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_INCLUDE_BASE_TRADE_H_
#define FT_INCLUDE_BASE_TRADE_H_

#include <string>

#include "Order.h"

namespace ft {

struct Trade {
  // std::string trade_time;
  uint64_t ticker_index;
  uint64_t order_id;
  uint64_t trade_id;
  uint64_t trade_time;
  Direction direction;
  Offset offset;
  double price;
  int64_t volume;
};

}  // namespace ft

#endif  // FT_INCLUDE_BASE_TRADE_H_
