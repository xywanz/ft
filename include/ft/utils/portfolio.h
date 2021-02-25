// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_INCLUDE_FT_UTILS_PORTFOLIO_H_
#define FT_INCLUDE_FT_UTILS_PORTFOLIO_H_

#include <ft/base/trade_msg.h>
#include <ft/utils/redis_position_helper.h>

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace ft {

class Portfolio {
 public:
  Portfolio();

  void Init(uint32_t max_tid, bool sync_to_redis = false, uint64_t account = 0);

  void set_account(uint64_t account);

  void set_position(const Position& pos);

  void UpdatePending(uint32_t ticker_id, Direction direction, Offset offset, int changed);

  void UpdateTraded(uint32_t ticker_id, Direction direction, Offset offset, int traded,
                    double traded_price);

  void UpdateComponentStock(uint32_t ticker_id, int traded, bool acquire);

  void UpdateFloatPnl(uint32_t ticker_id, double bid, double ask);

  void UpdateOnQueryTrade(uint32_t ticker_id, Direction direction, Offset offset,
                          int closed_volume);

  const Position* get_position(uint32_t ticker_id) const {
    auto iter = positions_.find(ticker_id);
    if (iter == positions_.end()) {
      return nullptr;
    }
    return &iter->second;
  }

  double total_assets() const;

 private:
  void UpdateBuyOrSellPending(uint32_t ticker_id, Direction direction, Offset offset, int changed);

  void UpdatePurchaseOrRedeemPending(uint32_t ticker_id, Direction direction, int changed);

  void UpdateBuyOrSell(uint32_t ticker_id, Direction direction, Offset offset, int traded,
                       double traded_price);

  void UpdatePurchaseOrRedeem(uint32_t ticker_id, Direction direction, int traded);

 private:
  std::unordered_map<uint32_t, Position> positions_;
  std::unique_ptr<RedisPositionSetter> redis_;
};

}  // namespace ft

#endif  // FT_INCLUDE_FT_UTILS_PORTFOLIO_H_
