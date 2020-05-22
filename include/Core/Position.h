// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_INCLUDE_CORE_POSITION_H_
#define FT_INCLUDE_CORE_POSITION_H_

#include <cstdint>

namespace ft {

struct PositionDetail {
  int yd_holdings = 0;
  int holdings = 0;
  int frozen = 0;
  int open_pending = 0;
  int close_pending = 0;

  double cost_price = 0;
  double float_pnl = 0;
};

struct Position {
  uint32_t ticker_index = 0;
  PositionDetail long_pos;
  PositionDetail short_pos;
};

}  // namespace ft

#endif  // FT_INCLUDE_CORE_POSITION_H_
