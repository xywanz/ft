// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_INCLUDE_CORE_TRADE_H_
#define FT_INCLUDE_CORE_TRADE_H_

#include <cstdint>

namespace ft {

struct Trade {
  uint64_t ticker_index;
  uint64_t direction;
  uint64_t offset;
  int64_t volume;
  double price;
};

}  // namespace ft

#endif  // FT_INCLUDE_CORE_TRADE_H_
