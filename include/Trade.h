// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_INCLUDE_TRADE_H_
#define FT_INCLUDE_TRADE_H_

#include <string>

#include "Common.h"

namespace ft {

struct Trade {
  std::string     symbol;
  std::string     exchange;
  std::string     ticker;
  std::string     order_id;
  std::string     trade_id;
  std::string     trade_time;
  Direction       direction;
  Offset  offset;
  double          price;
  int             volume;
};

}  // namespace ft

#endif  // FT_INCLUDE_TRADE_H_
