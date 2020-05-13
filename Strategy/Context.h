// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_STRATEGY_CONTEXT_H_
#define FT_STRATEGY_CONTEXT_H_

#include <algorithm>
#include <string>
#include <vector>

#include "Core/Constants.h"
#include "Core/ContractTable.h"
#include "Core/Protocol.h"
#include "IPC/redis.h"
#include "TradingSystem/PositionManager.h"

namespace ft {

class AlgoTradeContext {
 public:
  AlgoTradeContext()
      : redis_order_("127.0.0.1", 6379), portfolio_("127.0.0.1", 6379) {}

  void buy_open(const std::string& ticker, int volume, double price,
                uint64_t type = OrderType::FAK) {
    send_order(ticker, volume, Direction::BUY, Offset::OPEN, type, price);
  }

  void buy_close(const std::string& ticker, int volume, double price,
                 uint64_t type = OrderType::FAK) {
    send_order(ticker, volume, Direction::BUY, Offset::CLOSE_TODAY, type,
               price);
  }

  void sell_open(const std::string& ticker, int volume, double price,
                 uint64_t type = OrderType::FAK) {
    send_order(ticker, volume, Direction::SELL, Offset::OPEN, type, price);
  }

  void sell_close(const std::string& ticker, int volume, double price,
                  uint64_t type = OrderType::FAK) {
    send_order(ticker, volume, Direction::SELL, Offset::CLOSE_TODAY, type,
               price);
  }

  void cancel_order(uint64_t order_id) {
    TraderCommand cmd;
    memset(&cmd, 0, sizeof(cmd));
    cmd.type = CANCEL_ORDER;
    cmd.cancel_req.order_id = order_id;
    redis_order_.publish(TRADER_CMD_TOPIC, &cmd, sizeof(cmd));
  }

  Position get_position(const std::string& ticker) const {
    return portfolio_.get_position(ticker);
  }

  double get_realized_pnl() const { return portfolio_.get_realized_pnl(); }

  double get_float_pnl() const { return portfolio_.get_float_pnl(); }

 private:
  void send_order(const std::string& ticker, int volume, uint64_t direction,
                  uint64_t offset, uint64_t type, double price) {
    spdlog::info(
        "[AlgoTradeContext::send_order] ticker: {}, volume: {}, price: {}, "
        "type: {}, direction: {}, offset: {}",
        ticker, volume, price, ordertype_str(type), direction_str(direction),
        offset_str(offset));

    auto contract = ContractTable::get_by_ticker(ticker);
    assert(contract);

    TraderCommand cmd;
    memset(&cmd, 0, sizeof(cmd));
    cmd.type = NEW_ORDER;
    cmd.order_req.ticker_index = contract->index;
    cmd.order_req.volume = volume;
    cmd.order_req.direction = direction;
    cmd.order_req.offset = offset;
    cmd.order_req.type = type;
    cmd.order_req.price = price;

    redis_order_.publish(TRADER_CMD_TOPIC, &cmd, sizeof(cmd));
  }

 private:
  RedisSession redis_order_;
  PositionManager portfolio_;
};

}  // namespace ft

#endif  // FT_STRATEGY_CONTEXT_H_
