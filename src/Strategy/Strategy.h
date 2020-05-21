// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_SRC_STRATEGY_STRATEGY_H_
#define FT_SRC_STRATEGY_STRATEGY_H_

#include <chrono>
#include <memory>
#include <string>
#include <vector>

#include "Core/Constants.h"
#include "Core/Contract.h"
#include "Core/ContractTable.h"
#include "Core/Position.h"
#include "Core/Protocol.h"
#include "Core/TickData.h"
#include "IPC/redis.h"

namespace ft {

class Strategy {
 public:
  Strategy() {
    strategy_id_ = static_cast<uint32_t>(
        std::chrono::high_resolution_clock::now().time_since_epoch().count());
  }

  virtual ~Strategy() {}

  virtual void on_init() {}

  virtual void on_tick(const TickData* tick) {}

  virtual void on_exit() {}

  void run() {
    on_init();

    std::thread rsp_receiver([this] {
      for (;;) {
        auto reply = rsp_redis_.get_sub_reply();
        if (reply) {
          auto tick = reinterpret_cast<const TickData*>(reply->element[2]->str);
          on_tick(tick);
        }
      }
    });

    for (;;) {
      auto reply = tick_redis_.get_sub_reply();
      if (reply) {
        auto tick = reinterpret_cast<const TickData*>(reply->element[2]->str);
        on_tick(tick);
      }
    }
  }

 protected:
  void subscribe(const std::vector<std::string>& sub_list) {
    std::vector<std::string> topics;
    for (const auto& ticker : sub_list)
      topics.emplace_back(proto_md_topic(ticker));
    tick_redis_.subscribe(topics);
  }

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
    Position pos;

    auto reply = portfolio_redis_.get(proto_pos_key(ticker));
    if (reply->len == 0) return pos;

    memcpy(&pos, reply->str, sizeof(pos));
    return pos;
  }

  double get_realized_pnl() const {
    auto reply = portfolio_redis_.get("realized_pnl");
    if (reply->len == 0) return 0;
    return *reinterpret_cast<double*>(reply->str);
  }

  double get_float_pnl() const {
    auto reply = portfolio_redis_.get("float_pnl");
    if (reply->len == 0) return 0;
    return *reinterpret_cast<double*>(reply->str);
  }

 private:
  void send_order(const std::string& ticker, int volume, uint64_t direction,
                  uint64_t offset, uint64_t type, double price) {
    spdlog::info(
        "[Strategy::send_order] ticker: {}, volume: {}, price: {}, "
        "type: {}, direction: {}, offset: {}",
        ticker, volume, price, ordertype_str(type), direction_str(direction),
        offset_str(offset));

    const Contract* contract;
    contract = ContractTable::get_by_ticker(ticker);
    assert(contract);

    TraderCommand cmd{};
    cmd.magic = TRADER_CMD_MAGIC;
    cmd.type = NEW_ORDER;
    cmd.strategy_id = strategy_id_;
    cmd.order_req.ticker_index = contract->index;
    cmd.order_req.volume = volume;
    cmd.order_req.direction = direction;
    cmd.order_req.offset = offset;
    cmd.order_req.type = type;
    cmd.order_req.price = price;

    cmd_redis_.publish(TRADER_CMD_TOPIC, &cmd, sizeof(cmd));
  }

 private:
  uint32_t strategy_id_;

  RedisSession tick_redis_;
  RedisSession cmd_redis_;
  RedisSession rsp_redis_;
  RedisSession portfolio_redis_;
};

#define EXPORT_STRATEGY(type) \
  extern "C" ft::Strategy* create_strategy() { return new type; }

}  // namespace ft

#endif  // FT_SRC_STRATEGY_STRATEGY_H_
