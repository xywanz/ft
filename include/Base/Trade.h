// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_INCLUDE_BASE_TRADE_H_
#define FT_INCLUDE_BASE_TRADE_H_

#include <string>

#include "Order.h"

namespace ft {

struct Trade {
  std::string symbol;
  std::string exchange;
  std::string ticker;
  std::string order_id;
  std::string trade_time;
  int64_t trade_id;
  Direction direction;
  Offset offset;
  double price;
  int64_t volume;
};

}  // namespace ft

#endif  // FT_INCLUDE_BASE_TRADE_H_
