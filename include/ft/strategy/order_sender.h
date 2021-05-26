// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_INCLUDE_FT_STRATEGY_ORDER_SENDER_H_
#define FT_INCLUDE_FT_STRATEGY_ORDER_SENDER_H_

#include <string>

#include "ft/base/contract_table.h"
#include "ft/base/trade_msg.h"
#include "ft/component/yijinjing/journal/JournalWriter.h"

namespace ft {

class OrderSender {
 public:
  OrderSender() {}

  void Init(const std::string& trade_mq_name) {
    cmd_sender_ = yijinjing::JournalWriter::create(".", trade_mq_name, "order_sender");
  }

  void SetOrderFlag(OrderFlag flags) { flags_ = flags; }

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
    cmd.without_check = false;
    strncpy(cmd.strategy_id, strategy_id_, sizeof(cmd.strategy_id));
    cmd.order_req.client_order_id = client_order_id;
    cmd.order_req.ticker_id = ticker_id;
    cmd.order_req.volume = volume;
    cmd.order_req.direction = direction;
    cmd.order_req.offset = offset;
    cmd.order_req.type = type;
    cmd.order_req.price = price;
    cmd.order_req.flags = flags_;

    std::string buf;
    cmd.SerializeToString(&buf);
    cmd_sender_->write_str(buf, 0);
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
    cmd.without_check = false;
    cmd.cancel_req.order_id = order_id;

    std::string buf;
    cmd.SerializeToString(&buf);
    cmd_sender_->write_str(buf, 0);
  }

  void CancelForTicker(const std::string& ticker) {
    auto contract = ContractTable::get_by_ticker(ticker);
    assert(contract);
    TraderCommand cmd{};
    cmd.magic = kTradingCmdMagic;
    cmd.type = CMD_CANCEL_TICKER;
    cmd.without_check = false;
    cmd.cancel_ticker_req.ticker_id = contract->ticker_id;

    std::string buf;
    cmd.SerializeToString(&buf);
    cmd_sender_->write_str(buf, 0);
  }

  void CancelAll() {
    TraderCommand cmd{};
    cmd.magic = kTradingCmdMagic;
    cmd.type = CMD_CANCEL_ALL;
    cmd.without_check = false;

    std::string buf;
    cmd.SerializeToString(&buf);
    cmd_sender_->write_str(buf, 0);
  }

  void SendNotification(uint64_t signal) {
    TraderCommand cmd{};
    cmd.magic = kTradingCmdMagic;
    cmd.type = CMD_NOTIFY;
    cmd.notification.signal = signal;

    std::string buf;
    cmd.SerializeToString(&buf);
    cmd_sender_->write_str(buf, 0);
  }

 private:
  StrategyIdType strategy_id_;
  yijinjing::JournalWriterPtr cmd_sender_;
  std::string ft_cmd_topic_;
  OrderFlag flags_{0};
};

}  // namespace ft

#endif  // FT_INCLUDE_FT_STRATEGY_ORDER_SENDER_H_
