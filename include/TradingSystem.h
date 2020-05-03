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

#include "Base/DataStruct.h"
#include "Base/EventEngine.h"
#include "GeneralApi.h"
#include "MarketData/TickDatabase.h"
#include "RiskManagement/RiskManager.h"
#include "TradingInfo/TradingPanel.h"

namespace ft {

class TradingSystem {
 public:
  TradingSystem();

  ~TradingSystem();

  void close();

  bool login(const LoginParams& params);

  std::string buy_open(const std::string& ticker, int volume, OrderType type, double price) {
    return send_order(ticker, volume, Direction::BUY, Offset::OPEN, type, price);
  }

  std::string sell_close(const std::string& ticker, int volume, OrderType type, double price) {
    return send_order(ticker, volume, Direction::SELL, Offset::CLOSE_TODAY, type, price);
  }

  std::string sell_open(const std::string& ticker, int volume, OrderType type, double price) {
    return send_order(ticker, volume, Direction::SELL, Offset::OPEN, type, price);
  }

  std::string buy_close(const std::string& ticker, int volume, OrderType type, double price) {
    return send_order(ticker, volume, Direction::BUY, Offset::CLOSE_TODAY, type, price);
  }

  bool cancel_order(const std::string& order_id);

  // void show_positions();

  // callback

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

  void on_sync(cppex::Any*);

 private:
  std::string send_order(const std::string& ticker, int volume,
                         Direction direction, Offset offset,
                         OrderType type, double price);

 private:
  enum EventType {
    EV_SYNC = EV_USER_EVENT_START,
  };

  std::unique_ptr<EventEngine> engine_ = nullptr;
  std::unique_ptr<GeneralApi> api_ = nullptr;

  RiskManager risk_mgr_;
  TradingPanel panel_;
  std::map<std::string, TickDatabase> tick_datahub_;

  std::atomic<bool> is_process_pos_done_ = false;
  mutable std::mutex mutex_;
};

}  // namespace ft

#endif  // FT_INCLUDE_TRADINGSYSTEM_H_
