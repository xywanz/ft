// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_INCLUDE_FT_STRATEGY_ORDER_SENDER_H_
#define FT_INCLUDE_FT_STRATEGY_ORDER_SENDER_H_

#include <ft/base/contract_table.h>
#include <ft/base/trade_msg.h>
#include <ft/utils/redis_trader_cmd_helper.h>

#include <string>

namespace ft {

class OrderSender {
 public:
  void set_id(const std::string& name) {
    strncpy(strategy_id_, name.c_str(), sizeof(strategy_id_) - 1);
  }

  void set_account(uint64_t account_id) { cmd_pusher_.set_account(account_id); }

  void set_order_flags(OrderFlag flags) { flags_ = flags; }

  void BuyOpen(const std::string& ticker, int volume, double price,
               OrderType type = OrderType::kFak, uint32_t client_order_id = 0) {
    SendOrder(ticker, volume, Direction::kBuy, Offset::kOpen, type, price, client_order_id);
  }

  void BuyOpen(uint32_t ticker_id, int volume, double price, OrderType type = OrderType::kFak,
               uint32_t client_order_id = 0) {
    SendOrder(ticker_id, volume, Direction::kBuy, Offset::kOpen, type, price, client_order_id);
  }

  void BuyClose(const std::string& ticker, int volume, double price,
                OrderType type = OrderType::kFak, uint32_t client_order_id = 0) {
    SendOrder(ticker, volume, Direction::kBuy, Offset::kCloseToday, type, price, client_order_id);
  }

  void BuyClose(uint32_t ticker_id, int volume, double price, OrderType type = OrderType::kFak,
                uint32_t client_order_id = 0) {
    SendOrder(ticker_id, volume, Direction::kBuy, Offset::kCloseToday, type, price,
              client_order_id);
  }

  void SellOpen(const std::string& ticker, int volume, double price,
                OrderType type = OrderType::kFak, uint32_t client_order_id = 0) {
    SendOrder(ticker, volume, Direction::kSell, Offset::kOpen, type, price, client_order_id);
  }

  void SellOpen(uint32_t ticker_id, int volume, double price, OrderType type = OrderType::kFak,
                uint32_t client_order_id = 0) {
    SendOrder(ticker_id, volume, Direction::kSell, Offset::kOpen, type, price, client_order_id);
  }

  void SellClose(const std::string& ticker, int volume, double price,
                 OrderType type = OrderType::kFak, uint32_t client_order_id = 0) {
    SendOrder(ticker, volume, Direction::kSell, Offset::kCloseToday, type, price, client_order_id);
  }

  void SellClose(uint32_t ticker_id, int volume, double price, OrderType type = OrderType::kFak,
                 uint32_t client_order_id = 0) {
    SendOrder(ticker_id, volume, Direction::kSell, Offset::kCloseToday, type, price,
              client_order_id);
  }

  void SendOrder(uint32_t ticker_id, int volume, Direction direction, Offset offset, OrderType type,
                 double price, uint32_t client_order_id) {
    TraderCommand cmd{};
    cmd.magic = kTradingCmdMagic;
    cmd.type = CMD_NEW_ORDER;
    strncpy(cmd.strategy_id, strategy_id_, sizeof(cmd.strategy_id));
    cmd.order_req.client_order_id = client_order_id;
    cmd.order_req.ticker_id = ticker_id;
    cmd.order_req.volume = volume;
    cmd.order_req.direction = direction;
    cmd.order_req.offset = offset;
    cmd.order_req.type = type;
    cmd.order_req.price = price;
    cmd.order_req.flags = flags_;
    cmd.order_req.without_check = false;

    cmd_pusher_.Push(cmd);
  }

  void SendOrder(const std::string& ticker, int volume, Direction direction, Offset offset,
                 OrderType type, double price, uint32_t client_order_id) {
    const Contract* contract;
    contract = ContractTable::get_by_ticker(ticker);
    assert(contract);

    SendOrder(contract->ticker_id, volume, direction, offset, type, price, client_order_id);
  }

  void CancelOrder(uint64_t order_id) {
    TraderCommand cmd{};
    cmd.magic = kTradingCmdMagic;
    cmd.type = CMD_CANCEL_ORDER;
    cmd.cancel_req.order_id = order_id;

    cmd_pusher_.Push(cmd);
  }

  void CancelForTicker(const std::string& ticker) {
    auto contract = ContractTable::get_by_ticker(ticker);
    assert(contract);
    TraderCommand cmd{};
    cmd.magic = kTradingCmdMagic;
    cmd.type = CMD_CANCEL_TICKER;
    cmd.cancel_ticker_req.ticker_id = contract->ticker_id;

    cmd_pusher_.Push(cmd);
  }

  void CancelAll() {
    TraderCommand cmd{};
    cmd.magic = kTradingCmdMagic;
    cmd.type = CMD_CANCEL_ALL;

    cmd_pusher_.Push(cmd);
  }

  void SendNotification(uint64_t signal) {
    TraderCommand cmd{};
    cmd.magic = kTradingCmdMagic;
    cmd.type = CMD_NOTIFY;
    cmd.notification.signal = signal;

    cmd_pusher_.Push(cmd);
  }

 private:
  StrategyIdType strategy_id_;
  RedisTraderCmdPusher cmd_pusher_;
  OrderFlag flags_{0};
};

}  // namespace ft

#endif  // FT_INCLUDE_FT_STRATEGY_ORDER_SENDER_H_
