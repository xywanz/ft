// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_SRC_STRATEGY_STRATEGY_H_
#define FT_SRC_STRATEGY_STRATEGY_H_

#include <memory>
#include <string>
#include <vector>

#include "cep/data/constants.h"
#include "cep/data/contract.h"
#include "cep/data/contract_table.h"
#include "cep/data/position.h"
#include "cep/data/protocol.h"
#include "cep/data/tick_data.h"
#include "ipc/redis.h"
#include "ipc/redis_md_helper.h"
#include "ipc/redis_position_helper.h"
#include "strategy/order_sender.h"

namespace ft {

class Strategy {
 public:
  Strategy() {}

  virtual ~Strategy() {}

  virtual void on_init() {}

  virtual void on_tick(const TickData& tick) {}

  virtual void on_order_rsp(const OrderResponse& order) {}

  virtual void on_exit() {}

  /* 仅供加载器调用，内部不可使用 */
  void run();

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
  void subscribe(const std::vector<std::string>& sub_list);

  void buy_open(const std::string& ticker, int volume, double price,
                uint64_t type = OrderType::FAK, uint32_t user_order_id = 0) {
    sender_.send_order(ticker, volume, Direction::BUY, Offset::OPEN, type,
                       price, user_order_id);
  }

  void buy_close(const std::string& ticker, int volume, double price,
                 uint64_t type = OrderType::FAK, uint32_t user_order_id = 0) {
    sender_.send_order(ticker, volume, Direction::BUY, Offset::CLOSE_TODAY,
                       type, price, user_order_id);
  }

  void sell_open(const std::string& ticker, int volume, double price,
                 uint64_t type = OrderType::FAK, uint32_t user_order_id = 0) {
    sender_.send_order(ticker, volume, Direction::SELL, Offset::OPEN, type,
                       price, user_order_id);
  }

  void sell_close(const std::string& ticker, int volume, double price,
                  uint64_t type = OrderType::FAK, uint32_t user_order_id = 0) {
    sender_.send_order(ticker, volume, Direction::SELL, Offset::CLOSE_TODAY,
                       type, price, user_order_id);
  }

  void cancel_order(uint64_t order_id) { sender_.cancel_order(order_id); }

  void cancel_for_ticker(const std::string& ticker) {
    sender_.cancel_for_ticker(ticker);
  }

  void cancel_all() { sender_.cancel_all(); }

  Position get_position(const std::string& ticker) const {
    Position pos{};
    pos_getter_.get(ticker, &pos);
    return pos;
  }

 private:
  void send_order(const std::string& ticker, int volume, uint32_t direction,
                  uint32_t offset, uint32_t type, double price,
                  uint32_t user_order_id) {
    sender_.send_order(ticker, volume, direction, offset, type, price,
                       user_order_id);
  }

 private:
  StrategyIdType strategy_id_;
  OrderSender sender_;
  RedisPositionGetter pos_getter_;
  RedisTERspPuller puller_;
};

#define EXPORT_STRATEGY(type) \
  extern "C" ft::Strategy* create_strategy() { return new type; }

}  // namespace ft

#endif  // FT_SRC_STRATEGY_STRATEGY_H_
