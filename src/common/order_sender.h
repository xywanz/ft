// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_SRC_COMMON_ORDER_SENDER_H_
#define FT_SRC_COMMON_ORDER_SENDER_H_

#include <mutex>
#include <string>

#include "core/constants.h"
#include "core/contract_table.h"
#include "core/protocol.h"
#include "ipc/redis.h"

namespace ft {

class OrderSender {
 public:
  void set_id(const std::string& name) {
    strncpy(strategy_id_, name.c_str(), sizeof(strategy_id_) - 1);
  }

  void set_account(uint64_t account_id) { proto_.set_account(account_id); }

  void buy_open(const std::string& ticker, int volume, double price,
                uint64_t type = OrderType::FAK, uint32_t user_order_id = 0) {
    send_order(ticker, volume, Direction::BUY, Offset::OPEN, type, price,
               user_order_id);
  }

  void buy_close(const std::string& ticker, int volume, double price,
                 uint64_t type = OrderType::FAK, uint32_t user_order_id = 0) {
    send_order(ticker, volume, Direction::BUY, Offset::CLOSE_TODAY, type, price,
               user_order_id);
  }

  void sell_open(const std::string& ticker, int volume, double price,
                 uint64_t type = OrderType::FAK, uint32_t user_order_id = 0) {
    send_order(ticker, volume, Direction::SELL, Offset::OPEN, type, price,
               user_order_id);
  }

  void sell_close(const std::string& ticker, int volume, double price,
                  uint64_t type = OrderType::FAK, uint32_t user_order_id = 0) {
    send_order(ticker, volume, Direction::SELL, Offset::CLOSE_TODAY, type,
               price, user_order_id);
  }

  void send_order(const std::string& ticker, int volume, uint32_t direction,
                  uint32_t offset, uint32_t type, double price,
                  uint32_t user_order_id) {
    const Contract* contract;
    contract = ContractTable::get_by_ticker(ticker);
    assert(contract);

    TraderCommand cmd{};
    cmd.magic = TRADER_CMD_MAGIC;
    cmd.type = NEW_ORDER;
    strncpy(cmd.strategy_id, strategy_id_, sizeof(cmd.strategy_id));
    cmd.order_req.user_order_id = user_order_id;
    cmd.order_req.ticker_index = contract->index;
    cmd.order_req.volume = volume;
    cmd.order_req.direction = direction;
    cmd.order_req.offset = offset;
    cmd.order_req.type = type;
    cmd.order_req.price = price;

    std::unique_lock<std::mutex> lock(mutex_);
    cmd_redis_.publish(proto_.trader_cmd_topic(), &cmd, sizeof(cmd));
  }

  void cancel_order(uint64_t order_id) {
    TraderCommand cmd{};
    cmd.magic = TRADER_CMD_MAGIC;
    cmd.type = CANCEL_ORDER;
    cmd.cancel_req.order_id = order_id;

    std::unique_lock<std::mutex> lock(mutex_);
    cmd_redis_.publish(proto_.trader_cmd_topic(), &cmd, sizeof(cmd));
  }

  void cancel_for_ticker(const std::string& ticker) {
    auto contract = ContractTable::get_by_ticker(ticker);
    assert(contract);
    TraderCommand cmd{};
    cmd.magic = TRADER_CMD_MAGIC;
    cmd.type = CANCEL_TICKER;
    cmd.cancel_ticker_req.ticker_index = contract->index;

    std::unique_lock<std::mutex> lock(mutex_);
    cmd_redis_.publish(proto_.trader_cmd_topic(), &cmd, sizeof(cmd));
  }

  void cancel_all() {
    TraderCommand cmd{};
    cmd.magic = TRADER_CMD_MAGIC;
    cmd.type = CANCEL_ALL;

    std::unique_lock<std::mutex> lock(mutex_);
    cmd_redis_.publish(proto_.trader_cmd_topic(), &cmd, sizeof(cmd));
  }

 private:
  StrategyIdType strategy_id_;
  RedisSession cmd_redis_;
  std::mutex mutex_;
  ProtocolQueryCenter proto_;
};

}  // namespace ft

#endif  // FT_SRC_STRATEGY_ORDER_SENDER_H_
