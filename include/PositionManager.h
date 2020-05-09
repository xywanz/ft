// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_INCLUDE_POSITIONMANAGER_H_
#define FT_INCLUDE_POSITIONMANAGER_H_

#include <hiredis.h>

#include <map>
#include <memory>
#include <string>

#include "Base/DataStruct.h"
#include "IPC/redis.h"

namespace ft {

class PositionManager {
 public:
  PositionManager(const std::string& ip, int port);

  Position get_position(const std::string& ticker) const;

  double get_realized_pnl() const;

  double get_float_pnl() const;

  void set_position(const Position* pos);

  void update_pending(uint64_t ticker_index, Direction direction, Offset offset,
                      int changed);

  void update_traded(uint64_t ticker_index, Direction direction, Offset offset,
                     int64_t traded, double traded_price);

  void update_float_pnl(uint64_t ticker_index, double last_price);

 private:
  static std::string get_pos_key(const std::string& ticker) {
    return fmt::format("pos-{}", ticker);
  }

  static std::string get_pnl_key() { return "realized_pnl"; }

  Position* find(uint64_t ticker_index) {
    auto iter = pos_map_.find(ticker_index);
    if (iter == pos_map_.end()) return nullptr;
    return &iter->second;
  }

  const Position* find(uint64_t ticker_index) const {
    return const_cast<PositionManager*>(this)->find(ticker_index);
  }

  Position& find_or_create_pos(uint64_t ticker_index) {
    auto& pos = pos_map_[ticker_index];
    pos.ticker_index = ticker_index;
    return pos;
  }

 private:
  RedisSession redis_sess_;
  std::map<uint64_t, Position> pos_map_;
  double realized_pnl_ = 0;
};

}  // namespace ft

#endif  // FT_INCLUDE_POSITIONMANAGER_H_
