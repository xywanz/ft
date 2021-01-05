// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_SRC_STRATEGY_STRATEGY_H_
#define FT_SRC_STRATEGY_STRATEGY_H_

#include <memory>
#include <string>
#include <vector>

#include "ipc/redis.h"
#include "ipc/redis_md_helper.h"
#include "ipc/redis_position_helper.h"
#include "strategy/order_sender.h"
#include "trading_server/datastruct/constants.h"
#include "trading_server/datastruct/contract.h"
#include "trading_server/datastruct/contract_table.h"
#include "trading_server/datastruct/position.h"
#include "trading_server/datastruct/protocol.h"
#include "trading_server/datastruct/tick_data.h"

namespace ft {

class Strategy {
 public:
  Strategy() {}

  virtual ~Strategy() {}

  virtual void OnInit() {}

  virtual void OnTick(const TickData& tick) {}

  virtual void OnOrderResponse(const OrderResponse& order) {}

  virtual void OnExit() {}

  /* 仅供加载器调用，内部不可使用 */
  void Run();

  /* 策略启动后请勿更改id */
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

  void BuyOpen(const std::string& ticker, int volume, double price, uint64_t type = OrderType::FAK,
               uint32_t client_order_id = 0) {
    sender_.SendOrder(ticker, volume, Direction::BUY, Offset::OPEN, type, price, client_order_id);
  }

  void BuyClose(const std::string& ticker, int volume, double price, uint64_t type = OrderType::FAK,
                uint32_t client_order_id = 0) {
    sender_.SendOrder(ticker, volume, Direction::BUY, Offset::CLOSE_TODAY, type, price,
                      client_order_id);
  }

  void SellOpen(const std::string& ticker, int volume, double price, uint64_t type = OrderType::FAK,
                uint32_t client_order_id = 0) {
    sender_.SendOrder(ticker, volume, Direction::SELL, Offset::OPEN, type, price, client_order_id);
  }

  void SellClose(const std::string& ticker, int volume, double price,
                 uint64_t type = OrderType::FAK, uint32_t client_order_id = 0) {
    sender_.SendOrder(ticker, volume, Direction::SELL, Offset::CLOSE_TODAY, type, price,
                      client_order_id);
  }

  void CancelOrder(uint64_t order_id) { sender_.CancelOrder(order_id); }

  void CancelForTicker(const std::string& ticker) { sender_.CancelForTicker(ticker); }

  void CancelAll() { sender_.CancelAll(); }

  Position get_position(const std::string& ticker) const {
    Position pos{};
    pos_getter_.get(ticker, &pos);
    return pos;
  }

 private:
  void SendOrder(const std::string& ticker, int volume, uint32_t direction, uint32_t offset,
                 uint32_t type, double price, uint32_t client_order_id) {
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
