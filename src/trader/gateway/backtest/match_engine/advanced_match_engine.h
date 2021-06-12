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

// 估算订单位置双边成交量来确定挂单是否成交
class AdvancedMatchEngine : public MatchEngine {
 public:
  bool Init() override;

  bool InsertOrder(const OrderRequest& order) override;

  bool CancelOrder(uint64_t order_id, uint32_t ticker_id) override;

  void OnNewTick(const TickData& tick) override;

 private:
  struct InnerOrder {
    OrderRequest orig_order;
    int queue_position;
  };

  using BidOrderBook = std::map<uint64_t, std::list<InnerOrder>, std::greater<uint64_t>>;
  using AskOrderBook = std::map<uint64_t, std::list<InnerOrder>, std::less<uint64_t>>;

 private:
  std::vector<BidOrderBook> bid_orderbooks_;
  std::vector<AskOrderBook> ask_orderbooks_;
  // order_id -> price_u64：用于撤单时能快速找到订单队列
  std::map<uint64_t, uint64_t> id_price_map_;
  std::vector<TickData> ticks_;
};

}  // namespace ft
