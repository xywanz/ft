// Copyright [2020-present] <Copyright Kevin, kevin.lau.gd@gmail.com>

#pragma once

#include <algorithm>
#include <functional>
#include <list>
#include <map>
#include <vector>

#include "ft/base/market_data.h"
#include "trader/gateway/backtest/match_engine/match_engine.h"

namespace ft {

inline uint64_t u64_price(double p) {
  return (static_cast<uint64_t>(p * 100000000UL) + 50UL) / 10000UL;
}

class AdvancedMatchEngine : public MatchEngine {
 public:
  bool Init() override;

  bool InsertOrder(const OrderRequest& order) override;

  bool CancelOrder(uint64_t order_id, uint32_t ticker_id) override;

  void OnNewTick(const TickData& tick) override;

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
