// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_INCLUDE_POSITION_H_
#define FT_INCLUDE_POSITION_H_

#include <atomic>
#include <map>
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

  Position(const Position& other)
    : symbol(other.symbol),
      exchange(other.exchange),
      ticker(other.ticker),
      direction(other.direction),
      yd_volume(other.yd_volume),
      volume(other.volume),
      frozen(other.frozen),
      open_pending(other.open_pending.load()),
      close_pending(other.close_pending.load()),
      price(other.price),
      pnl(other.pnl) {
  }

  std::string symbol;
  std::string exchange;
  std::string ticker;
  Direction direction;

  int yd_volume = 0;
  int volume = 0;
  int frozen = 0;

  std::atomic<int> open_pending = 0;
  std::atomic<int> close_pending = 0;

  double price = 0;
  double pnl = 0;
};


class PositionManager {
 public:
  PositionManager();

 private:
  std::map<std::string, Position> pos_map_;
};

}  // namespace ft

#endif  // FT_INCLUDE_POSITION_H_
