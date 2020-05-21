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

  void update_pending(uint32_t ticker_index, uint32_t direction,
                      uint32_t offset, int changed);

  void update_traded(uint32_t ticker_index, uint32_t direction, uint32_t offset,
                     int traded, double traded_price);

  void update_float_pnl(uint32_t ticker_index, double last_price);

  void update_on_query_trade(uint32_t ticker_index, uint32_t direction,
                             uint32_t offset, int closed_volume);

 private:
  Position* find(uint32_t ticker_index) {
    auto iter = pos_map_.find(ticker_index);
    if (iter == pos_map_.end()) return nullptr;
    return &iter->second;
  }

  const Position* find(uint32_t ticker_index) const {
    return const_cast<PositionManager*>(this)->find(ticker_index);
  }

  Position& find_or_create_pos(uint32_t ticker_index) {
    auto& pos = pos_map_[ticker_index];
    pos.ticker_index = ticker_index;
    return pos;
  }

 private:
  RedisSession redis_;
  std::map<uint32_t, Position> pos_map_;
  double realized_pnl_ = 0;
};

}  // namespace ft

#endif  // FT_TRADINGSYSTEM_POSITIONMANAGER_H_
