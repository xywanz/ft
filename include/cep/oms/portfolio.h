// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_SRC_COMMON_PORTFOLIO_H_
#define FT_SRC_COMMON_PORTFOLIO_H_

#include <memory>
#include <string>
#include <unordered_map>

#include "cep/data/position.h"
#include "cep/data/protocol.h"
#include "ipc/redis_position_helper.h"

namespace ft {

class Portfolio {
 public:
  explicit Portfolio(bool sync_to_redis = false);

  void set_account(uint64_t account);

  void set_position(const Position& pos);

  void update_pending(uint32_t tid, uint32_t direction, uint32_t offset,
                      int changed);

  void update_traded(uint32_t tid, uint32_t direction, uint32_t offset,
                     int traded, double traded_price);

  void update_component_stock(uint32_t tid, int traded, bool acquire);

  void update_float_pnl(uint32_t tid, double last_price);

  void update_on_query_trade(uint32_t tid, uint32_t direction, uint32_t offset,
                             int closed_volume);

  const Position* find(uint32_t tid) const {
    return const_cast<Portfolio*>(this)->find(tid);
  }

 private:
  void update_buy_or_sell_pending(uint32_t tid, uint32_t direction,
                                  uint32_t offset, int changed);

  void update_purchase_or_redeem_pending(uint32_t tid, uint32_t direction,
                                         int changed);

  void update_buy_or_sell(uint32_t tid, uint32_t direction, uint32_t offset,
                          int traded, double traded_price);

  void update_purchase_or_redeem(uint32_t tid, uint32_t direction, int traded);

 private:
  Position* find(uint32_t tid) {
    auto iter = pos_map_.find(tid);
    if (iter == pos_map_.end()) return nullptr;
    return &iter->second;
  }

  Position& find_or_create_pos(uint32_t tid) {
    auto& pos = pos_map_[tid];
    pos.tid = tid;
    return pos;
  }

 private:
  std::unordered_map<uint32_t, Position> pos_map_;
  std::unique_ptr<RedisPositionSetter> redis_;
};

}  // namespace ft

#endif  // FT_SRC_COMMON_PORTFOLIO_H_
