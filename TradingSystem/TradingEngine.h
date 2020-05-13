// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_TRADINGSYSTEM_TRADINGENGINE_H_
#define FT_TRADINGSYSTEM_TRADINGENGINE_H_

#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "Core/Account.h"
#include "Core/Gateway.h"
#include "Core/LoginParams.h"
#include "Core/TradingEngineInterface.h"
#include "IPC/redis.h"
#include "TradingSystem/Order.h"
#include "TradingSystem/PositionManager.h"

namespace ft {

class TradingEngine : public TradingEngineInterface {
 public:
  TradingEngine();

  ~TradingEngine();

  bool login(const LoginParams& params);

  void run();

  void close();

 private:
  uint64_t send_order(uint64_t ticker_index, int volume, uint64_t direction,
                      uint64_t offset, uint64_t type, double price);

  void cancel_order(uint64_t order_id);

  void cancel_all_by_ticker(const std::string& ticker);

  void cancel_all();

  void on_query_contract(const Contract* contract) override;

  void on_query_account(const Account* account) override;

  void on_query_position(const Position* position) override;

  void on_tick(const TickData* tick) override;

  void on_order_accepted(uint64_t order_id) override;

  void on_order_rejected(uint64_t order_id) override;

  void on_order_traded(uint64_t order_id, int64_t this_traded,
                       double traded_price) override;

  void on_order_canceled(uint64_t order_id, int64_t canceled_volume) override;

  void on_order_cancel_rejected(uint64_t order_id) override;

 private:
  std::unique_ptr<Gateway> gateway_ = nullptr;

  PositionManager portfolio_;
  std::map<uint64_t, Order> order_map_;
  std::mutex mutex_;

  RedisSession tick_redis_;
  RedisSession order_redis_;

  std::atomic<bool> is_logon_ = false;
};

}  // namespace ft

#endif  // FT_TRADINGSYSTEM_TRADINGENGINE_H_
