// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_INCLUDE_POSITION_H_
#define FT_INCLUDE_POSITION_H_

#include <string>

#include "Common.h"

namespace ft {

struct Position {
  Position() {
  }

  Position(const std::string& _symbol,
           const std::string& _exchange,
           Direction _direction)
    : symbol(_symbol),
      exchange(_exchange),
      ticker(to_ticker(_symbol, _exchange)),
      direction(_direction) {
  }

  std::string symbol;
  std::string exchange;
  std::string ticker;
  Direction direction;
  int yd_volume = 0;
  int volume = 0;
  int frozen = 0;
  double price = 0;
  double pnl = 0;

  int open_pending = 0;
  int close_pending = 0;
};

}  // namespace ft

#endif  // FT_INCLUDE_POSITION_H_
