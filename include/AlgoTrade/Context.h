// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_INCLUDE_ALGOTRADE_CONTEXT_H_
#define FT_INCLUDE_ALGOTRADE_CONTEXT_H_

#include <algorithm>
#include <string>
#include <vector>

#include "AlgoTrade/StrategyEngine.h"
#include "IPC/redis.h"
#include "PositionManager.h"

namespace ft {

class AlgoTradeContext {
 public:
  AlgoTradeContext()
      : redis_order_("127.0.0.1", 6379), portfolio_("127.0.0.1", 6379) {}

  void buy_open(const std::string& ticker, int volume, double price,
                OrderType type = OrderType::FAK) {
    send_order(ticker, volume, Direction::BUY, Offset::OPEN, type, price);
  }

  void buy_close(const std::string& ticker, int volume, double price,
                 OrderType type = OrderType::FAK) {
    send_order(ticker, volume, Direction::BUY, Offset::CLOSE_TODAY, type,
               price);
  }

  void sell_open(const std::string& ticker, int volume, double price,
                 OrderType type = OrderType::FAK) {
    send_order(ticker, volume, Direction::SELL, Offset::OPEN, type, price);
  }

  void sell_close(const std::string& ticker, int volume, double price,
                  OrderType type = OrderType::FAK) {
    send_order(ticker, volume, Direction::SELL, Offset::CLOSE_TODAY, type,
               price);
  }

  void cancel_order(uint64_t order_id) {
    redis_order_.publish("cancel_order", &order_id, sizeof(order_id));
  }

  Position get_position(const std::string& ticker) const {
    return portfolio_.get_position(ticker);
  }

  double get_realized_pnl() const { return portfolio_.get_realized_pnl(); }

  double get_float_pnl() const { return portfolio_.get_float_pnl(); }

 private:
  void send_order(const std::string& ticker, int volume, Direction direction,
                  Offset offset, OrderType type, double price) {
    auto contract = ContractTable::get_by_ticker(ticker);
    assert(contract);

    Order order;
    order.ticker_index = contract->index;
    order.volume = volume;
    order.direction = direction;
    order.offset = offset;
    order.type = type;
    order.price = price;

    redis_order_.publish("send_order", &order, sizeof(order));
  }

 private:
  RedisSession redis_order_;
  PositionManager portfolio_;
};

}  // namespace ft

#endif  // FT_INCLUDE_ALGOTRADE_CONTEXT_H_
