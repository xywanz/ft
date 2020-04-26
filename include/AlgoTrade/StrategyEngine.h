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
#include "EventEngine.h"
#include "GeneralApi.h"
#include "MdManager.h"
#include "RiskManagement/RiskManager.h"
#include "TradingManagement/TradingView.h"

namespace ft {

class Strategy;

class StrategyEngine {
 public:
  explicit StrategyEngine(FrontType front_type);

  ~StrategyEngine();

  void close();

  bool login(const LoginParams& params);

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

  void cancel_all(const std::string& ticker = "") {
    std::vector<const std::string*> order_id_list;
    get_order_id_list(&order_id_list, ticker);
    for (const auto& order_id : order_id_list)
      cancel_order(*order_id);
  }

  void mount_strategy(const std::string& ticker, Strategy* strategy);

  void unmount_strategy(Strategy* strategy);

  void get_order_id_list(std::vector<const std::string*>* out,
                         const std::string& ticker = "") const {
    trading_view_.get_order_id_list(out, ticker);
  }

  void get_order_list(std::vector<const Order*>* out,
                      const std::string& ticker = "") const {
    trading_view_.get_order_list(out, ticker);
  }

  const Account* get_account() const {
    return trading_view_.get_account();
  }

  const Position* get_position(const std::string& ticker) const {
    return trading_view_.get_position(ticker);
  }

  void get_pos_ticker_list(std::vector<const std::string*>* out) const {
    trading_view_.get_pos_ticker_list(out);
  }

  const MarketData* get_tick(const std::string& ticker, std::size_t offset) const {
    auto iter = ticks_.find(ticker);
    if (iter == ticks_.end())
      return nullptr;

    auto& vec = iter->second;
    if (offset >= vec.size())
      return nullptr;
    return *(vec.rbegin() + offset);
  }

  // callback

  void on_mount_strategy(cppex::Any* data);

  void on_unmount_strategy(cppex::Any* data);

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
  bool send_order(const std::string& ticker, int volume,
                         Direction direction, Offset offset,
                         OrderType type, double price);

 private:
  enum EventType {
    EV_MOUNT_STRATEGY = EV_USER_EVENT_START,
    EV_UMOUNT_STRATEGY,
  };

  std::unique_ptr<EventEngine> engine_ = nullptr;
  std::unique_ptr<GeneralApi> api_ = nullptr;

  std::map<std::string, MdManager> md_center_;
  std::map<std::string, std::list<Strategy*>> strategies_;
  std::map<std::string, std::vector<MarketData*>> ticks_;

  TradingView trading_view_;
  RiskManager risk_mgr_;

  bool is_login_ = false;
  std::atomic<bool> is_process_pos_done_ = false;
};

}  // namespace ft

#endif  // FT_INCLUDE_ALGOTRADE_STRATEGYENGINE_H_
