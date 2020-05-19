// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_STRATEGY_CONTEXT_H_
#define FT_STRATEGY_CONTEXT_H_

#include <algorithm>
#include <string>
#include <vector>

#include "Core/Constants.h"
#include "Core/ContractTable.h"
#include "Core/Position.h"
#include "Core/Protocol.h"
#include "IPC/redis.h"

namespace ft {

class PositionHelper {
 public:
  PositionHelper() : redis_("127.0.0.1", 6379) {}

  Position get_position(const std::string& ticker) const {
    Position pos;

    auto reply = redis_.get(fmt::format("pos-{}", ticker));
    if (reply->len == 0) return pos;

    memcpy(&pos, reply->str, sizeof(pos));
    return pos;
  }

  double get_realized_pnl() const {
    auto reply = redis_.get("realized_pnl");
    if (reply->len == 0) return 0;
    return *reinterpret_cast<double*>(reply->str);
  }

  double get_float_pnl() const {
    auto reply = redis_.get("float_pnl");
    if (reply->len == 0) return 0;
    return *reinterpret_cast<double*>(reply->str);
  }

 private:
  RedisSession redis_;
};

class AlgoTradeContext {
 public:
  AlgoTradeContext() : cmd_redis_("127.0.0.1", 6379) {}

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
    TraderCommand cmd{};
    cmd.magic = TRADER_CMD_MAGIC;
    cmd.type = CANCEL_ORDER;
    cmd.cancel_req.order_id = order_id;
    cmd_redis_.publish(TRADER_CMD_TOPIC, &cmd, sizeof(cmd));
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

    const Contract* contract;
    if (ticker.find_first_of('.') != std::string::npos)
      contract = ContractTable::get_by_ticker(ticker);
    else
      contract = ContractTable::get_by_symbol(ticker);
    assert(contract);

    TraderCommand cmd{};
    cmd.magic = TRADER_CMD_MAGIC;
    cmd.type = NEW_ORDER;
    cmd.order_req.ticker_index = contract->index;
    cmd.order_req.volume = volume;
    cmd.order_req.direction = direction;
    cmd.order_req.offset = offset;
    cmd.order_req.type = type;
    cmd.order_req.price = price;

    cmd_redis_.publish(TRADER_CMD_TOPIC, &cmd, sizeof(cmd));
  }

 private:
  RedisSession cmd_redis_;
  PositionHelper portfolio_;
};

}  // namespace ft

#endif  // FT_STRATEGY_CONTEXT_H_
