// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_TRADINGSYSTEM_POSITIONMANAGER_H_
#define FT_TRADINGSYSTEM_POSITIONMANAGER_H_

#include <map>
#include <memory>
#include <string>

#include "Core/Position.h"
#include "IPC/redis.h"

namespace ft {

class PositionManager {
 public:
  PositionManager(const std::string& ip, int port);

  void set_position(const Position* pos);

  void update_pending(uint64_t ticker_index, uint64_t direction,
                      uint64_t offset, int changed);

  void update_traded(uint64_t ticker_index, uint64_t direction, uint64_t offset,
                     int64_t traded, double traded_price);

  void update_float_pnl(uint64_t ticker_index, double last_price);

 private:
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
  RedisSession redis_;
  std::map<uint64_t, Position> pos_map_;
  double realized_pnl_ = 0;
};

}  // namespace ft

#endif  // FT_TRADINGSYSTEM_POSITIONMANAGER_H_
