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
#include "Core/Config.h"
#include "Core/Gateway.h"
#include "Core/RiskManagementInterface.h"
#include "Core/TradingEngineInterface.h"
#include "IPC/redis.h"
#include "TradingSystem/Order.h"
#include "TradingSystem/PositionManager.h"

namespace ft {

class TradingEngine : public TradingEngineInterface {
 public:
  TradingEngine();

  ~TradingEngine();

  bool login(const Config& config);

  void run();

  void close();

 private:
  bool send_order(const TraderCommand* cmd);

  void cancel_order(uint64_t order_id);

  void cancel_all_for_ticker(uint32_t ticker_index);

  void cancel_all();

  void on_query_contract(const Contract* contract) override;

  void on_query_account(const Account* account) override;

  void on_query_position(const Position* position) override;

  void on_query_trade(const Trade* trade) override;

  void on_tick(const TickData* tick) override;

  void on_order_accepted(uint64_t order_id) override;

  void on_order_rejected(uint64_t order_id) override;

  void on_order_traded(uint64_t order_id, int this_traded,
                       double traded_price) override;

  void on_order_canceled(uint64_t order_id, int canceled_volume) override;

  void on_order_cancel_rejected(uint64_t order_id) override;

 private:
  uint64_t next_engine_order_id() { return next_engine_order_id_++; }

  void respond_send_order_error(const TraderCommand* cmd);

 private:
  std::unique_ptr<Gateway> gateway_ = nullptr;
  std::unique_ptr<RiskManagementInterface> risk_mgr_ = nullptr;

  PositionManager portfolio_;
  std::map<uint64_t, Order> order_map_;
  std::mutex mutex_;

  uint64_t next_engine_order_id_ = 1;

  RedisSession tick_redis_;
  RedisSession order_redis_;
  RedisSession rsp_redis_;

  std::atomic<bool> is_logon_ = false;
};

}  // namespace ft

#endif  // FT_TRADINGSYSTEM_TRADINGENGINE_H_
