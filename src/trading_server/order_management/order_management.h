// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_SRC_TRADING_SERVER_ORDER_MANAGEMENT_ORDER_MANAGEMENT_H_
#define FT_SRC_TRADING_SERVER_ORDER_MANAGEMENT_ORDER_MANAGEMENT_H_

#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "gateway/gateway.h"
#include "ipc/redis_md_helper.h"
#include "trading_server/datastruct/account.h"
#include "trading_server/datastruct/config.h"
#include "trading_server/datastruct/error_code.h"
#include "trading_server/datastruct/md_snapshot.h"
#include "trading_server/datastruct/order.h"
#include "trading_server/order_management/base_oms.h"
#include "trading_server/order_management/portfolio.h"
#include "trading_server/risk_management/risk_management.h"
#include "trading_server/risk_management/types.h"

namespace ft {

class OrderManagementSystem : public BaseOrderManagementSystem {
 public:
  OrderManagementSystem();
  ~OrderManagementSystem();

  bool Login(const Config& config);

  void ProcessCmd();

  void Close();

  static uint64_t version() { return 202008172355; }

 private:
  void ProcessRedisCmd();
  void ProcessQueueCmd();
  void ExecuteCmd(const TraderCommand& cmd);

  bool SendOrder(const TraderCommand& cmd);
  void CancelOrder(uint64_t order_id);
  void CancelForTicker(uint32_t tid);
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
  void HandleTimer();

  void OnPrimaryMarketTraded(Trade* rsp);    // ETF申赎
  void OnSecondaryMarketTraded(Trade* rsp);  // 二级市场买卖

  uint64_t next_order_id() { return next_oms_order_id_++; }

 private:
  std::unique_ptr<Gateway> gateway_{nullptr};

  volatile bool is_logon_{false};
  uint64_t next_oms_order_id_{1};

  Account account_;
  Portfolio portfolio_;
  OrderMap order_map_;
  std::unique_ptr<RiskManagementSystem> rms_{nullptr};
  RedisMdPusher md_pusher_;
  MarketDataSnashot md_snapshot_;
  std::mutex mutex_;
  std::unique_ptr<std::thread> timer_thread_;

  int cmd_queue_key_ = 0;
};

}  // namespace ft

#endif  // FT_SRC_TRADING_SERVER_ORDER_MANAGEMENT_ORDER_MANAGEMENT_H_
