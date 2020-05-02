// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_INCLUDE_ALGOTRADE_STRATEGYENGINE_H_
#define FT_INCLUDE_ALGOTRADE_STRATEGYENGINE_H_

#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "Base/DataStruct.h"
#include "cppex/Any.h"
#include "Base/EventEngine.h"
#include "GeneralApi.h"
#include "MarketData/Candlestick.h"
#include "MarketData/TickDatabase.h"
#include "RiskManagement/RiskManager.h"
#include "TradingInfo/TradingPanel.h"

namespace ft {

class Strategy;

class StrategyEngine {
 public:
  StrategyEngine();

  ~StrategyEngine();

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

  void cancel_all(const std::string& ticker = "") {
    std::vector<const std::string*> order_id_list;
    panel_.get_order_id_list(&order_id_list, ticker);
    for (const auto& order_id : order_id_list)
      cancel_order(*order_id);
  }

  void mount_strategy(const std::string& ticker, Strategy* strategy);

  void unmount_strategy(Strategy* strategy);

  const TickDatabase* get_tickdb(const std::string& ticker) const {
    auto iter = tick_datahub_.find(ticker);
    if (iter == tick_datahub_.end())
      return nullptr;
    return &iter->second;
  }

  const Candlestick* load_candlestick(const std::string& ticker) {
    auto db_iter = tick_datahub_.find(ticker);
    if (db_iter == tick_datahub_.end())
      return nullptr;

    auto iter = candle_charts_.find(ticker);
    if (iter != candle_charts_.end())
      return &iter->second;

    auto res = candle_charts_.emplace(ticker, Candlestick());
    res.first->second.on_init(&db_iter->second);
    return &res.first->second;
  }

  // callback

  void on_mount_strategy(cppex::Any* data);

  void on_unmount_strategy(cppex::Any* data);

  void on_sync(cppex::Any*);

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
  std::string send_order(const std::string& ticker, int volume,
                         Direction direction, Offset offset,
                         OrderType type, double price);

 private:
  enum EventType {
    EV_MOUNT_STRATEGY = EV_USER_EVENT_START,
    EV_UMOUNT_STRATEGY,
    EV_SYNC
  };

  std::unique_ptr<EventEngine> engine_ = nullptr;
  std::unique_ptr<GeneralApi> api_ = nullptr;

  std::map<std::string, TickDatabase> tick_datahub_;
  std::map<std::string, std::list<Strategy*>> strategies_;
  std::map<std::string, Candlestick> candle_charts_;

  TradingPanel panel_;
  RiskManager risk_mgr_;

  bool is_login_ = false;
  std::atomic<bool> is_process_pos_done_ = false;
};

}  // namespace ft

#endif  // FT_INCLUDE_ALGOTRADE_STRATEGYENGINE_H_
