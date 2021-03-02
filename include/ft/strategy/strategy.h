// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_SRC_STRATEGY_STRATEGY_H_
#define FT_SRC_STRATEGY_STRATEGY_H_

#include <ft/base/market_data.h>
#include <ft/base/trade_msg.h>
#include <ft/component/pubsub/subscriber.h>
#include <ft/strategy/order_sender.h>
#include <ft/utils/redis.h>
#include <ft/utils/redis_md_helper.h>
#include <ft/utils/redis_position_helper.h>

#include <string>
#include <vector>

namespace ft {

class Strategy {
 public:
  Strategy();

  virtual ~Strategy() {}

  virtual void OnInit() {}

  virtual void OnTick(const TickData& tick) {}

  virtual void OnOrderResponse(const OrderResponse& order) {}

  virtual void OnExit() {}

  // 仅供加载器调用，内部不可使用
  void Run();

  // 策略启动后请勿更改id
  void set_id(const std::string& name) {
    strncpy(strategy_id_, name.c_str(), sizeof(strategy_id_) - 1);
    sender_.set_id(name);
  }

  void set_account_id(uint64_t account_id) {
    sender_.set_account(account_id);
    pos_getter_.set_account(account_id);
  }

  void set_backtest_mode(bool backtest_mode = true) { backtest_mode_ = backtest_mode; }

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
  pubsub::Subscriber md_sub_;
  pubsub::Subscriber trade_msg_sub_;
  bool backtest_mode_ = false;
};

#define EXPORT_STRATEGY(type) \
  extern "C" ft::Strategy* CreateStrategy() { return new type; }

}  // namespace ft

#endif  // FT_SRC_STRATEGY_STRATEGY_H_
