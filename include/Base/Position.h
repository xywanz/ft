// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_INCLUDE_BASE_POSITION_H_
#define FT_INCLUDE_BASE_POSITION_H_

#include <spdlog/spdlog.h>

#include <atomic>
#include <map>
#include <mutex>
#include <string>
#include <vector>

#include "Base/Common.h"

namespace ft {

struct PositionDetail {
  int64_t yd_volume = 0;
  int64_t volume = 0;
  int64_t frozen = 0;
  int64_t open_pending = 0;
  int64_t close_pending = 0;
  double cost_price = 0;
  double float_pnl = 0;
};

struct Position {
  uint64_t ticker_index;
  PositionDetail long_pos;
  PositionDetail short_pos;
};

}  // namespace ft

#endif  // FT_INCLUDE_BASE_POSITION_H_
