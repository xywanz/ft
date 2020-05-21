// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_SRC_STRATEGY_STRATEGY_H_
#define FT_SRC_STRATEGY_STRATEGY_H_

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
  Strategy() {}

  virtual ~Strategy() {}

  virtual void on_init() {}

  virtual void on_tick(const TickData* tick) {}

  virtual void on_order_rsp(const OrderResponse* order) {}

  virtual void on_exit() {}

  /* 仅供加载器调用，内部不可使用 */
  void run();

  /* 策略启动后请勿更改id */
  void set_id(const std::string& name) {
    strncpy(strategy_id_, name.c_str(), sizeof(strategy_id_) - 1);
  }

 protected:
  void subscribe(const std::vector<std::string>& sub_list);

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
  void send_order(const std::string& ticker, int volume, uint32_t direction,
                  uint32_t offset, uint32_t type, double price,
                  uint32_t user_order_id);

 private:
  StrategyIdType strategy_id_;

  RedisSession tick_redis_;
  RedisSession cmd_redis_;
  RedisSession rsp_redis_;
  RedisSession portfolio_redis_;
};

#define EXPORT_STRATEGY(type) \
  extern "C" ft::Strategy* create_strategy() { return new type; }

}  // namespace ft

#endif  // FT_SRC_STRATEGY_STRATEGY_H_
