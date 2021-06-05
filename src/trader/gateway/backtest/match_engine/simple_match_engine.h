// Copyright [2020-present] <Copyright Kevin, kevin.lau.gd@gmail.com>

#pragma once

#include <map>
#include <vector>

#include "trader/gateway/backtest/match_engine/match_engine.h"

namespace ft {

class SimpleMatchEngine : public MatchEngine {
 public:
  bool Init() override;

  bool InsertOrder(const OrderRequest& order) override;

  bool CancelOrder(uint64_t order_id, uint32_t ticker_id) override;

  void OnNewTick(const TickData& tick) override;

 private:
  std::vector<std::map<uint64_t, OrderRequest>> orders_;  // ticker_id: order_id -> order
  std::vector<TickData> ticks_;

  uint64_t cur_timestamp_us_ = 0;
};

}  // namespace ft
