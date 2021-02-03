// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_SRC_STRATEGY_STRATEGY_H_
#define FT_SRC_STRATEGY_STRATEGY_H_

#include <memory>
#include <string>
#include <vector>

#include "strategy/order_sender.h"
#include "trading_server/datastruct/constants.h"
#include "trading_server/datastruct/contract.h"
#include "trading_server/datastruct/position.h"
#include "trading_server/datastruct/protocol.h"
#include "trading_server/datastruct/tick_data.h"
#include "utils/contract_table.h"
#include "utils/redis.h"
#include "utils/redis_md_helper.h"
#include "utils/redis_position_helper.h"

namespace ft {

class Strategy {
 public:
  Strategy() {}

  virtual ~Strategy() {}

  virtual void OnInit() {}

  virtual void OnTick(const TickData& tick) {}

  virtual void OnOrderResponse(const OrderResponse& order) {}

  virtual void OnExit() {}

  // 仅供加载器调用，内部不可使用
  void Run();

  void RunBackTest();

  // 策略启动后请勿更改id
  void set_id(const std::string& name) {
    strncpy(strategy_id_, name.c_str(), sizeof(strategy_id_) - 1);
    sender_.set_id(name);
  }

  void set_account_id(uint64_t account_id) {
    sender_.set_account(account_id);
    pos_getter_.set_account(account_id);
  }

 protected:
  void Subscribe(const std::vector<std::string>& sub_list);

  void BuyOpen(const std::string& ticker, int volume, double price,
               OrderType type = OrderType::kFak, uint32_t client_order_id = 0) {
    sender_.SendOrder(ticker, volume, Direction::kBuy, Offset::kOpen, type, price, client_order_id);
  }

  void BuyClose(const std::string& ticker, int volume, double price,
                OrderType type = OrderType::kFak, uint32_t client_order_id = 0) {
    sender_.SendOrder(ticker, volume, Direction::kBuy, Offset::kCloseToday, type, price,
                      client_order_id);
  }

  void SellOpen(const std::string& ticker, int volume, double price,
                OrderType type = OrderType::kFak, uint32_t client_order_id = 0) {
    sender_.SendOrder(ticker, volume, Direction::kSell, Offset::kOpen, type, price,
                      client_order_id);
  }

  void SellClose(const std::string& ticker, int volume, double price,
                 OrderType type = OrderType::kFak, uint32_t client_order_id = 0) {
    sender_.SendOrder(ticker, volume, Direction::kSell, Offset::kCloseToday, type, price,
                      client_order_id);
  }

  void CancelOrder(uint64_t order_id) { sender_.CancelOrder(order_id); }

  void CancelForTicker(const std::string& ticker) { sender_.CancelForTicker(ticker); }

  void CancelAll() { sender_.CancelAll(); }

  void SendNotification(uint64_t signal) { sender_.SendNotification(signal); }

  Position get_position(const std::string& ticker) const {
    Position pos{};
    pos_getter_.get(ticker, &pos);
    return pos;
  }

 private:
  void SendOrder(const std::string& ticker, int volume, Direction direction, Offset offset,
                 OrderType type, double price, uint32_t client_order_id) {
    sender_.SendOrder(ticker, volume, direction, offset, type, price, client_order_id);
  }

 private:
  StrategyIdType strategy_id_;
  OrderSender sender_;
  RedisPositionGetter pos_getter_;
  RedisTERspPuller puller_;
};

#define EXPORT_STRATEGY(type) \
  extern "C" ft::Strategy* CreateStrategy() { return new type; }

}  // namespace ft

#endif  // FT_SRC_STRATEGY_STRATEGY_H_
