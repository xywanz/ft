// Copyright [2020-present] <Copyright Kevin, kevin.lau.gd@gmail.com>

#pragma once

#include <functional>
#include <list>
#include <map>
#include <vector>

#include "match_engine.h"

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

  struct OrderBook {
    std::map<uint64_t, std::list<InnerOrder>, std::greater<uint64_t>> bid_levels;
    std::map<uint64_t, std::list<InnerOrder>, std::less<uint64_t>> ask_levels;
  };

 private:
  std::vector<OrderBook> orderbooks_;
  // order_id -> price_u64：用于撤单时能快速找到订单队列
  std::map<uint64_t, uint64_t> id_price_map_;
  std::vector<TickData> ticks_;
};

}  // namespace ft
