// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_SRC_COMMON_ORDER_SENDER_H_
#define FT_SRC_COMMON_ORDER_SENDER_H_

#include <string>

#include "core/constants.h"
#include "core/contract_table.h"
#include "core/protocol.h"
#include "ipc/redis_trader_cmd_helper.h"

namespace ft {

class OrderSender {
 public:
  void set_id(const std::string& name) {
    strncpy(strategy_id_, name.c_str(), sizeof(strategy_id_) - 1);
  }

  void set_account(uint64_t account_id) { cmd_pusher_.set_account(account_id); }

  void set_order_flags(uint32_t flags) { flags_ = flags; }

  void buy_open(const std::string& ticker, int volume, double price,
                uint64_t type = OrderType::FAK, uint32_t user_order_id = 0) {
    send_order(ticker, volume, Direction::BUY, Offset::OPEN, type, price,
               user_order_id);
  }

  void buy_open(uint32_t ticker_index, int volume, double price,
                uint64_t type = OrderType::FAK, uint32_t user_order_id = 0) {
    send_order(ticker_index, volume, Direction::BUY, Offset::OPEN, type, price,
               user_order_id);
  }

  void buy_close(const std::string& ticker, int volume, double price,
                 uint64_t type = OrderType::FAK, uint32_t user_order_id = 0) {
    send_order(ticker, volume, Direction::BUY, Offset::CLOSE_TODAY, type, price,
               user_order_id);
  }

  void buy_close(uint32_t ticker_index, int volume, double price,
                 uint64_t type = OrderType::FAK, uint32_t user_order_id = 0) {
    send_order(ticker_index, volume, Direction::BUY, Offset::CLOSE_TODAY, type,
               price, user_order_id);
  }

  void sell_open(const std::string& ticker, int volume, double price,
                 uint64_t type = OrderType::FAK, uint32_t user_order_id = 0) {
    send_order(ticker, volume, Direction::SELL, Offset::OPEN, type, price,
               user_order_id);
  }

  void sell_open(uint32_t ticker_index, int volume, double price,
                 uint64_t type = OrderType::FAK, uint32_t user_order_id = 0) {
    send_order(ticker_index, volume, Direction::SELL, Offset::OPEN, type, price,
               user_order_id);
  }

  void sell_close(const std::string& ticker, int volume, double price,
                  uint64_t type = OrderType::FAK, uint32_t user_order_id = 0) {
    send_order(ticker, volume, Direction::SELL, Offset::CLOSE_TODAY, type,
               price, user_order_id);
  }

  void sell_close(uint32_t ticker_index, int volume, double price,
                  uint64_t type = OrderType::FAK, uint32_t user_order_id = 0) {
    send_order(ticker_index, volume, Direction::SELL, Offset::CLOSE_TODAY, type,
               price, user_order_id);
  }

  void send_order(uint32_t ticker_index, int volume, uint32_t direction,
                  uint32_t offset, uint32_t type, double price,
                  uint32_t user_order_id) {
    TraderCommand cmd{};
    cmd.magic = TRADER_CMD_MAGIC;
    cmd.type = CMD_NEW_ORDER;
    strncpy(cmd.strategy_id, strategy_id_, sizeof(cmd.strategy_id));
    cmd.order_req.user_order_id = user_order_id;
    cmd.order_req.ticker_index = ticker_index;
    cmd.order_req.volume = volume;
    cmd.order_req.direction = direction;
    cmd.order_req.offset = offset;
    cmd.order_req.type = type;
    cmd.order_req.price = price;
    cmd.order_req.flags = flags_;
    cmd.order_req.without_check = false;

    cmd_pusher_.push(cmd);
  }

  void send_order(const std::string& ticker, int volume, uint32_t direction,
                  uint32_t offset, uint32_t type, double price,
                  uint32_t user_order_id) {
    const Contract* contract;
    contract = ContractTable::get_by_ticker(ticker);
    assert(contract);

    send_order(contract->index, volume, direction, offset, type, price,
               user_order_id);
  }

  void cancel_order(uint64_t order_id) {
    TraderCommand cmd{};
    cmd.magic = TRADER_CMD_MAGIC;
    cmd.type = CMD_CANCEL_ORDER;
    cmd.cancel_req.order_id = order_id;

    cmd_pusher_.push(cmd);
  }

  void cancel_for_ticker(const std::string& ticker) {
    auto contract = ContractTable::get_by_ticker(ticker);
    assert(contract);
    TraderCommand cmd{};
    cmd.magic = TRADER_CMD_MAGIC;
    cmd.type = CMD_CANCEL_TICKER;
    cmd.cancel_ticker_req.ticker_index = contract->index;

    cmd_pusher_.push(cmd);
  }

  void cancel_all() {
    TraderCommand cmd{};
    cmd.magic = TRADER_CMD_MAGIC;
    cmd.type = CMD_CANCEL_ALL;

    cmd_pusher_.push(cmd);
  }

 private:
  StrategyIdType strategy_id_;
  RedisTraderCmdPusher cmd_pusher_;
  uint32_t flags_{0};
};

}  // namespace ft

#endif  // FT_SRC_COMMON_ORDER_SENDER_H_
