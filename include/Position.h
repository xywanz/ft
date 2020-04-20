// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_INCLUDE_POSITION_H_
#define FT_INCLUDE_POSITION_H_

#include <atomic>
#include <map>
#include <string>

#include "Common.h"

namespace ft {

struct PositionDetail {
  PositionDetail() {}

  PositionDetail(const PositionDetail& other)
    : yd_volume(other.yd_volume),
      volume(other.volume),
      frozen(other.frozen),
      open_pending(other.open_pending.load()),
      close_pending(other.close_pending.load()),
      cost_price(other.cost_price),
      pnl(other.pnl) {}

  int64_t yd_volume = 0;
  int64_t volume = 0;
  int64_t frozen = 0;
  std::atomic<int64_t> open_pending = 0;
  std::atomic<int64_t> close_pending = 0;
  double cost_price = 0;
  double pnl = 0;
};

struct Position {
  Position() {}

  Position(const std::string& _symbol, const std::string& _exchange)
    : symbol(_symbol),
      exchange(_exchange),
      ticker(to_ticker(_symbol, _exchange)) {}

  Position(const Position& other)
    : symbol(other.symbol),
      exchange(other.exchange),
      ticker(other.ticker),
      long_pos(other.long_pos),
      short_pos(other.short_pos) {}

  std::string symbol;
  std::string exchange;
  std::string ticker;

  PositionDetail long_pos;
  PositionDetail short_pos;

  int64_t short_yd_volume = 0;
  int64_t short_volume = 0;
  int64_t short_frozen = 0;
  std::atomic<int64_t> short_open_pending = 0;
  std::atomic<int64_t> short_close_pending = 0;
  double short_cost_price = 0;
  double short_pnl = 0;
};


class PositionManager {
 public:
  PositionManager();

 private:
  std::map<std::string, Position> pos_map_;
};

}  // namespace ft

#endif  // FT_INCLUDE_POSITION_H_
