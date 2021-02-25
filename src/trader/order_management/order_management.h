// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_SRC_TRADER_ORDER_MANAGEMENT_ORDER_MANAGEMENT_H_
#define FT_SRC_TRADER_ORDER_MANAGEMENT_ORDER_MANAGEMENT_H_

#include <ft/base/error_code.h>
#include <ft/base/market_data.h>
#include <ft/base/trade_msg.h>
#include <ft/trader/base_oms.h>
#include <ft/trader/gateway.h>
#include <ft/utils/portfolio.h>
#include <ft/utils/redis_md_helper.h>

#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "trader/order.h"
#include "trader/risk_management/risk_management.h"

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
  void HandleTimer();

  void OnPrimaryMarketTraded(Trade* rsp);    // ETF申赎
  void OnSecondaryMarketTraded(Trade* rsp);  // 二级市场买卖

  uint64_t next_order_id() { return next_oms_order_id_++; }

 private:
  Gateway* gateway_{nullptr};
  GatewayDestroyFunc gateway_dtor_{nullptr};
  void* gateway_dl_handle_{nullptr};

  volatile bool is_logon_{false};
  uint64_t next_oms_order_id_{1};

  Account account_;
  Portfolio portfolio_;
  OrderMap order_map_;
  std::unique_ptr<RiskManagementSystem> rms_{nullptr};
  RedisMdPusher md_pusher_;
  std::mutex mutex_;
  std::unique_ptr<std::thread> timer_thread_;

  int cmd_queue_key_ = 0;
};

}  // namespace ft

#endif  // FT_SRC_TRADER_ORDER_MANAGEMENT_ORDER_MANAGEMENT_H_
