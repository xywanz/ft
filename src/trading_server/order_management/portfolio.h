// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_SRC_TRADING_SERVER_ORDER_MANAGEMENT_PORTFOLIO_H_
#define FT_SRC_TRADING_SERVER_ORDER_MANAGEMENT_PORTFOLIO_H_

#include <memory>
#include <string>
#include <vector>

#include "ipc/redis_position_helper.h"
#include "trading_server/datastruct/position.h"
#include "trading_server/datastruct/protocol.h"

namespace ft {

class Portfolio {
 public:
  Portfolio();

  void Init(uint32_t max_tid, bool sync_to_redis = false, uint64_t account = 0);

  void set_account(uint64_t account);

  void set_position(const Position& pos);

  void UpdatePending(uint32_t tid, uint32_t direction, uint32_t offset, int changed);

  void UpdateTraded(uint32_t tid, uint32_t direction, uint32_t offset, int traded,
                    double traded_price);

  void UpdateComponentStock(uint32_t tid, int traded, bool acquire);

  void UpdateFloatPnl(uint32_t tid, double last_price);

  void UpdateOnQueryTrade(uint32_t tid, uint32_t direction, uint32_t offset, int closed_volume);

  const Position* get_position(uint32_t tid) const {
    if (tid == 0 || tid >= positions_.size()) return nullptr;
    return &positions_[tid];
  }

 private:
  void UpdateBuyOrSellPending(uint32_t tid, uint32_t direction, uint32_t offset, int changed);

  void UpdatePurchaseOrRedeemPending(uint32_t tid, uint32_t direction, int changed);

  void UpdateBuyOrSell(uint32_t tid, uint32_t direction, uint32_t offset, int traded,
                       double traded_price);

  void UpdatePurchaseOrRedeem(uint32_t tid, uint32_t direction, int traded);

 private:
  std::vector<Position> positions_;
  std::unique_ptr<RedisPositionSetter> redis_;
};

}  // namespace ft

#endif  // FT_SRC_TRADING_SERVER_ORDER_MANAGEMENT_PORTFOLIO_H_
