// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_INCLUDE_TRADINGSYSTEM_H_
#define FT_INCLUDE_TRADINGSYSTEM_H_

#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "Account.h"
#include "Common.h"
#include "Contract.h"
#include "EventEngine.h"
#include "GeneralApi.h"
#include "MarketData.h"
#include "MdManager.h"
#include "Order.h"
#include "Position.h"
#include "Trade.h"

namespace ft {

class Strategy;

class TradingSystem {
 public:
  explicit TradingSystem(FrontType front_type);

  ~TradingSystem();

  bool login(const LoginParams& params);

  std::size_t subscribe(const std::vector<std::string>& tickers);

  bool buy_open(const std::string& ticker, int volume, OrderType type, double price) {
    return send_order(ticker, volume, Direction::BUY, Offset::OPEN, type, price);
  }

  bool sell_close(const std::string& ticker, int volume, OrderType type, double price) {
    return send_order(ticker, volume, Direction::SELL, Offset::CLOSE_TODAY, type, price);
  }

  bool sell_open(const std::string& ticker, int volume, OrderType type, double price) {
    return send_order(ticker, volume, Direction::SELL, Offset::OPEN, type, price);
  }

  bool buy_close(const std::string& ticker, int volume, OrderType type, double price) {
    return send_order(ticker, volume, Direction::BUY, Offset::CLOSE_TODAY, type, price);
  }

  bool cancel_order(const std::string& order_id);

  void show_positions();

  void on_show_position(cppex::Any*);

  bool mount_strategy(const std::string& ticker, Strategy *strategy);

  void join() {
    api_->join();
  }

  void on_mount_strategy(cppex::Any* data);

  /*
   * 接收查询到的汇总仓位数据
   * 当gateway触发了一次仓位查询时，需要把仓位缓存并根据{ticker-direction}
   * 进行汇总，每个{ticker-direction}对应一个Position对象，本次查询完成后，
   * 对每个汇总的Position对象回调Trader::on_position
   */
  void on_position(cppex::Any* data);

  /*
   * 接收查询到的账户信息
   */
  void on_account(cppex::Any* data);

  /*
   * 接受订单信息
   * 当订单状态发生改变时触发
   */
  void on_order(cppex::Any* data);

  /*
   * 接受成交信息
   * 每笔成交都会回调
   */
  void on_trade(cppex::Any* data);

  /*
   * 接受行情数据
   * 每一个tick都会回调
   */
  void on_tick(cppex::Any* data);

 private:
  static std::string to_pos_key(const std::string& ticker, Direction direction) {
    return fmt::format("{}_{}", ticker, to_string(direction));
  }

  bool send_order(const std::string& ticker, int volume,
                         Direction direction, Offset offset,
                         OrderType type, double price);

  void update_volume(const std::string& ticker,
                     Direction direction,
                     Offset offset,
                     int traded,
                     int pending_changed);

  void update_pnl(const std::string& ticker, double last_price);

  void handle_canceled(const Order* rtn_order);
  void handle_submitted(const Order* rtn_order);
  void handle_part_traded(const Order* rtn_order);
  void handle_all_traded(const Order* rtn_order);
  void handle_cancel_rejected(const Order* rtn_order);

 private:
  enum EventType {
    EV_MOUNT_STRATEGY = EV_USER_EVENT_START,
    EV_SHOW_POSITION
  };

  std::unique_ptr<EventEngine> engine_ = nullptr;
  std::unique_ptr<GeneralApi> api_ = nullptr;

  Account account_;

  std::map<std::string, Position> positions_;  // ticker_direction->position
  std::map<std::string, std::vector<Trade>> trade_record_;
  std::map<std::string, Order> orders_;  // order_id->order
  std::map<std::string, MdManager> md_center_;
  std::map<std::string, std::list<Strategy*>> strategies_;

  mutable std::mutex order_mutex_;

  bool is_login_ = false;
};

}  // namespace ft

#endif  // FT_INCLUDE_TRADINGSYSTEM_H_
