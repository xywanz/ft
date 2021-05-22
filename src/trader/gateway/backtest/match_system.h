// Copyright [2020-2021] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_SRC_TRADER_GATEWAY_BACKTEST_MATCH_SYSTEM_H_
#define FT_SRC_TRADER_GATEWAY_BACKTEST_MATCH_SYSTEM_H_

#include <algorithm>
#include <list>
#include <map>
#include <vector>

#include "ft/base/market_data.h"
#include "trader/msg.h"

namespace ft {

inline uint64_t u64_price(double p) {
  return (static_cast<uint64_t>(p * 100000000UL) + 50UL) / 10000UL;
}

class MatchSystem {
 public:
  void UpdateTick(const TickData& new_tick);

  void AddOrder(const OrderRequest& order);

  void DelOrder(uint64_t order_id);

 private:
  void AddLongOrder(const OrderRequest& order);
  void AddShortOrder(const OrderRequest& order);

 private:
  struct InnerOrder {
    OrderRequest orig_order;
    int traded;
    int order_pos;
  };

  TickData current_tick_;
  std::vector<OrderRequest> orders_;

  std::map<uint64_t, std::list<InnerOrder>, std::greater<uint64_t>> bid_levels_;
  std::map<uint64_t, std::list<InnerOrder>, std::less<uint64_t>> ask_levels_;
};

}  // namespace ft

#endif  // FT_SRC_TRADER_GATEWAY_BACKTEST_MATCH_SYSTEM_H_
