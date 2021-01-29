// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_SRC_CEP_COMMON_ORDER_SENDER_H_
#define FT_SRC_CEP_COMMON_ORDER_SENDER_H_

#include <string>

#include "utils/redis_trader_cmd_helper.h"
#include "trading_server/datastruct/constants.h"
#include "trading_server/datastruct/contract_table.h"
#include "trading_server/datastruct/protocol.h"

namespace ft {

class OrderSender {
 public:
  void set_id(const std::string& name) {
    strncpy(strategy_id_, name.c_str(), sizeof(strategy_id_) - 1);
  }

  void set_account(uint64_t account_id) { cmd_pusher_.set_account(account_id); }

  void set_order_flags(uint32_t flags) { flags_ = flags; }

  void BuyOpen(const std::string& ticker, int volume, double price, uint64_t type = OrderType::FAK,
               uint32_t client_order_id = 0) {
    SendOrder(ticker, volume, Direction::BUY, Offset::OPEN, type, price, client_order_id);
  }

  void BuyOpen(uint32_t tid, int volume, double price, uint64_t type = OrderType::FAK,
               uint32_t client_order_id = 0) {
    SendOrder(tid, volume, Direction::BUY, Offset::OPEN, type, price, client_order_id);
  }

  void BuyClose(const std::string& ticker, int volume, double price, uint64_t type = OrderType::FAK,
                uint32_t client_order_id = 0) {
    SendOrder(ticker, volume, Direction::BUY, Offset::CLOSE_TODAY, type, price, client_order_id);
  }

  void BuyClose(uint32_t tid, int volume, double price, uint64_t type = OrderType::FAK,
                uint32_t client_order_id = 0) {
    SendOrder(tid, volume, Direction::BUY, Offset::CLOSE_TODAY, type, price, client_order_id);
  }

  void SellOpen(const std::string& ticker, int volume, double price, uint64_t type = OrderType::FAK,
                uint32_t client_order_id = 0) {
    SendOrder(ticker, volume, Direction::SELL, Offset::OPEN, type, price, client_order_id);
  }

  void SellOpen(uint32_t tid, int volume, double price, uint64_t type = OrderType::FAK,
                uint32_t client_order_id = 0) {
    SendOrder(tid, volume, Direction::SELL, Offset::OPEN, type, price, client_order_id);
  }

  void SellClose(const std::string& ticker, int volume, double price,
                 uint64_t type = OrderType::FAK, uint32_t client_order_id = 0) {
    SendOrder(ticker, volume, Direction::SELL, Offset::CLOSE_TODAY, type, price, client_order_id);
  }

  void SellClose(uint32_t tid, int volume, double price, uint64_t type = OrderType::FAK,
                 uint32_t client_order_id = 0) {
    SendOrder(tid, volume, Direction::SELL, Offset::CLOSE_TODAY, type, price, client_order_id);
  }

  void SendOrder(uint32_t tid, int volume, uint32_t direction, uint32_t offset, uint32_t type,
                 double price, uint32_t client_order_id) {
    TraderCommand cmd{};
    cmd.magic = TRADER_CMD_MAGIC;
    cmd.type = CMD_NEW_ORDER;
    strncpy(cmd.strategy_id, strategy_id_, sizeof(cmd.strategy_id));
    cmd.order_req.client_order_id = client_order_id;
    cmd.order_req.tid = tid;
    cmd.order_req.volume = volume;
    cmd.order_req.direction = direction;
    cmd.order_req.offset = offset;
    cmd.order_req.type = type;
    cmd.order_req.price = price;
    cmd.order_req.flags = flags_;
    cmd.order_req.without_check = false;

    cmd_pusher_.Push(cmd);
  }

  void SendOrder(const std::string& ticker, int volume, uint32_t direction, uint32_t offset,
                 uint32_t type, double price, uint32_t client_order_id) {
    const Contract* contract;
    contract = ContractTable::get_by_ticker(ticker);
    assert(contract);

    SendOrder(contract->tid, volume, direction, offset, type, price, client_order_id);
  }

  void CancelOrder(uint64_t order_id) {
    TraderCommand cmd{};
    cmd.magic = TRADER_CMD_MAGIC;
    cmd.type = CMD_CANCEL_ORDER;
    cmd.cancel_req.order_id = order_id;

    cmd_pusher_.Push(cmd);
  }

  void CancelForTicker(const std::string& ticker) {
    auto contract = ContractTable::get_by_ticker(ticker);
    assert(contract);
    TraderCommand cmd{};
    cmd.magic = TRADER_CMD_MAGIC;
    cmd.type = CMD_CANCEL_TICKER;
    cmd.cancel_ticker_req.tid = contract->tid;

    cmd_pusher_.Push(cmd);
  }

  void CancelAll() {
    TraderCommand cmd{};
    cmd.magic = TRADER_CMD_MAGIC;
    cmd.type = CMD_CANCEL_ALL;

    cmd_pusher_.Push(cmd);
  }

 private:
  StrategyIdType strategy_id_;
  RedisTraderCmdPusher cmd_pusher_;
  uint32_t flags_{0};
};

}  // namespace ft

#endif  // FT_SRC_CEP_COMMON_ORDER_SENDER_H_
