// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_SRC_TRADER_OMS_H_
#define FT_SRC_TRADER_OMS_H_

#include <list>
#include <map>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "ft/base/error_code.h"
#include "ft/base/market_data.h"
#include "ft/base/trade_msg.h"
#include "ft/component/position_calculator.h"
#include "ft/component/pubsub/publisher.h"
#include "ft/trader/base_oms.h"
#include "ft/trader/gateway.h"
#include "ft/utils/redis_position_helper.h"
#include "ft/utils/spinlock.h"
#include "ft/utils/timer_thread.h"
#include "trader/order.h"
#include "trader/risk/rms.h"

namespace ft {

// 当前不支持销毁
class OrderManagementSystem : public BaseOrderManagementSystem {
 public:
  OrderManagementSystem();

  bool Login(const Config& config);

  void ProcessCmd();

 private:
  void ProcessPubSubCmd();
  void ProcessQueueCmd();
  void ExecuteCmd(const TraderCommand& cmd);

  bool SendOrder(const TraderCommand& cmd);
  void CancelOrder(uint64_t order_id);
  void CancelForTicker(uint32_t ticker_id);
  void CancelAll();

  void OnTick(TickData* tick) override;

  void OnOrderAccepted(OrderAcceptance* rsp) override;
  void OnOrderRejected(OrderRejection* rsp) override;
  void OnOrderTraded(Trade* rsp) override;
  void OnOrderCanceled(OrderCancellation* rsp) override;
  void OnOrderCancelRejected(OrderCancelRejection* rsp) override;

 private:
  void HandleAccount(Account* account);
  void HandlePositions(std::vector<Position>* positions);
  void HandleTrades(std::vector<Trade>* trades);
  bool HandleTimer();

  void OnPrimaryMarketTraded(Trade* rsp);    // ETF申赎
  void OnSecondaryMarketTraded(Trade* rsp);  // 二级市场买卖

  uint64_t next_order_id() { return next_oms_order_id_++; }

 private:
  Gateway* gateway_{nullptr};

  volatile bool is_logon_{false};
  uint64_t next_oms_order_id_{1};

  SpinLock spinlock_;
  Account account_;
  PositionCalculator pos_calculator_;
  RedisPositionSetter redis_pos_updater_;
  OrderMap order_map_;
  std::unique_ptr<RiskManagementSystem> rms_{nullptr};
  pubsub::Publisher md_pusher_;
  TimerThread timer_thread_;

  int cmd_queue_key_ = 0;
};

}  // namespace ft

#endif  // FT_SRC_TRADER_OMS_H_
