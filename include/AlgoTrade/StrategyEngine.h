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
#include "Base/EventEngine.h"
#include "DataCenter.h"
#include "Gateway.h"
#include "IPC/redis.h"
#include "MarketData/Candlestick.h"
#include "MarketData/TickDB.h"
#include "RiskManagement/RiskManager.h"
#include "TradingPanel.h"
#include "cppex/Any.h"

namespace ft {

class StrategyEngine {
 public:
  StrategyEngine();

  ~StrategyEngine();

  bool login(const LoginParams& params);

  void run();

  void close();

 private:
  uint64_t send_order(uint64_t ticker_index, int volume, uint64_t direction,
                      uint64_t offset, uint64_t type, double price);

  bool cancel_order(uint64_t order_id);

  void cancel_all(const std::string& ticker = "") {
    std::vector<uint64_t> order_id_list;
    panel_.get_order_id_list(&order_id_list, ticker);
    for (const auto& order_id : order_id_list) cancel_order(order_id);
  }

  // callback

  void process_sync(cppex::Any*);

  /*
   * 接收查询到的汇总仓位数据
   * 当gateway触发了一次仓位查询时，需要把仓位缓存并根据{ticker-direction}
   * 进行汇总，每个{ticker-direction}对应一个Position对象，本次查询完成后，
   * 对每个汇总的Position对象回调Trader::on_position
   */
  void process_position(cppex::Any* data);

  /*
   * 接收查询到的账户信息
   */
  void process_account(cppex::Any* data);

  /*
   * 接受订单信息
   * 当订单状态发生改变时触发
   */
  void process_order(cppex::Any* data);

  /*
   * 接受成交信息
   * 每笔成交都会回调
   */
  void process_trade(cppex::Any* data);

  /*
   * 接受行情数据
   * 每一个tick都会回调
   */
  void process_tick(cppex::Any* data);

 private:
  enum EventType { EV_SYNC = EV_USER_EVENT_START };

  std::unique_ptr<EventEngine> engine_ = nullptr;
  std::unique_ptr<Gateway> gateway_ = nullptr;

  TradingPanel panel_;
  RiskManager risk_mgr_;
  RedisSession redis_tick_;
  RedisSession redis_order_;

  std::atomic<bool> is_logon_ = false;

  std::mutex mutex_;  // 锁住仓位和订单
};

}  // namespace ft

#endif  // FT_INCLUDE_ALGOTRADE_STRATEGYENGINE_H_
