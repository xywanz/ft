// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_INCLUDE_CORE_TRADE_H_
#define FT_INCLUDE_CORE_TRADE_H_

#include <cstdint>

namespace ft {

struct Trade {
  uint32_t ticker_index;
  uint32_t direction;
  uint32_t offset;
  int volume;
  double price;
};

}  // namespace ft

#endif  // FT_INCLUDE_CORE_TRADE_H_
