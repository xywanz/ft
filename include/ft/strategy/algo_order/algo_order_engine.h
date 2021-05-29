// Copyright [2020-2021] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_INCLUDE_FT_STRATEGY_ALGO_ORDER_ALGO_ORDER_ENGINE_H_
#define FT_INCLUDE_FT_STRATEGY_ALGO_ORDER_ALGO_ORDER_ENGINE_H_

#include <cassert>
#include <string>

#include "ft/base/market_data.h"
#include "ft/base/trade_msg.h"
#include "ft/component/trader_db.h"
#include "ft/strategy/order_sender.h"

namespace ft {

class AlgoOrderEngine {
 public:
  virtual ~AlgoOrderEngine() {}

  virtual void Init() {}

  virtual void OnTick(const TickData& tick) {}

  virtual void OnOrder(const OrderResponse& order) {}

  virtual void OnTrade(const OrderResponse& trade) {}

  void SetStrategyName(const std::string& strategy_name) { strategy_name_ = strategy_name; }

  void SetOrderSender(OrderSender* order_sender) { order_sender_ = order_sender; }

  void SetTraderDB(TraderDB* trader_db) { trader_db_ = trader_db; }

  void SendOrder(uint32_t ticker_id, int volume, Direction direction, Offset offset, OrderType type,
                 double price, uint32_t client_order_id) {
    assert(order_sender_);
    order_sender_->SendOrder(ticker_id, volume, direction, offset, type, price, client_order_id);
  }

  void CancelOrder(uint32_t order_id) { order_sender_->CancelOrder(order_id); }

  Position GetPosition(const std::string& ticker) const {
    Position ret{};

    assert(trader_db_);
    trader_db_->GetPosition(strategy_name_, ticker, &ret);
    return ret;
  }

 private:
  std::string strategy_name_;
  OrderSender* order_sender_;
  TraderDB* trader_db_;
};

}  // namespace ft

#endif  // FT_INCLUDE_FT_STRATEGY_ALGO_ORDER_ALGO_ORDER_ENGINE_H_
