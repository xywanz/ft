// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_SRC_COMMON_PORTFOLIO_H_
#define FT_SRC_COMMON_PORTFOLIO_H_

#include <memory>
#include <string>
#include <vector>

#include "cep/data/position.h"
#include "cep/data/protocol.h"
#include "ipc/redis_position_helper.h"

namespace ft {

class Portfolio {
 public:
  Portfolio();

  void init(uint32_t max_tid, bool sync_to_redis = false, uint64_t account = 0);

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

  const Position* get_position(uint32_t tid) const {
    if (tid == 0 || tid >= positions_.size()) return nullptr;
    return &positions_[tid];
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
  std::vector<Position> positions_;
  std::unique_ptr<RedisPositionSetter> redis_;
};

}  // namespace ft

#endif  // FT_SRC_COMMON_PORTFOLIO_H_
