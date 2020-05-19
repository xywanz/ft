// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_INCLUDE_CORE_POSITION_H_
#define FT_INCLUDE_CORE_POSITION_H_

#include <cstdint>

namespace ft {

struct PositionDetail {
  int64_t yd_position = 0;
  int64_t holdings = 0;
  int64_t frozen = 0;
  int64_t open_pending = 0;
  int64_t close_pending = 0;
  int64_t td_closed = 0;
  double cost_price = 0;
  double float_pnl = 0;
};

struct Position {
  uint64_t ticker_index;
  PositionDetail long_pos;
  PositionDetail short_pos;
};

}  // namespace ft

#endif  // FT_INCLUDE_CORE_POSITION_H_
