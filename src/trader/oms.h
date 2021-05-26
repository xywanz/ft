// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_SRC_TRADER_OMS_H_
#define FT_SRC_TRADER_OMS_H_

#include <list>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <thread>
#include <vector>

#include "ft/base/error_code.h"
#include "ft/base/market_data.h"
#include "ft/base/trade_msg.h"
#include "ft/component/position_calculator.h"
#include "ft/component/yijinjing/journal/JournalReader.h"
#include "ft/component/yijinjing/journal/JournalWriter.h"
#include "ft/utils/redis_position_helper.h"
#include "ft/utils/spinlock.h"
#include "ft/utils/timer_thread.h"
#include "trader/gateway/gateway.h"
#include "trader/order.h"
#include "trader/risk/rms.h"

namespace ft {

// 当前不支持销毁
class OrderManagementSystem {
 public:
  OrderManagementSystem();

  bool Init(const FlareTraderConfig& config);

  void ProcessCmd();

  void operator()(const OrderAcceptance& rsp);
  void operator()(const OrderRejection& rsp);
  void operator()(const Trade& rsp);
  void operator()(const OrderCancellation& rsp);
  void operator()(const OrderCancelRejection& rsp);

 private:
  void ExecuteCmd(const TraderCommand& cmd);

  bool SendOrder(const TraderCommand& cmd);
  void DoCancelOrder(const Order& order, bool without_check);
  void CancelOrder(uint64_t order_id, bool without_check);
  void CancelForTicker(uint32_t ticker_id, bool without_check);
  void CancelAll(bool without_check);

  bool InitGateway();
  bool InitContractTable();
  bool InitAccount();
  bool InitPositions();
  bool InitTradeInfo();
  bool InitRMS();
  bool InitMQ();
  bool SubscribeMarketData();

  void SendRspToStrategy(const Order& order, int this_traded, double price, int error_code);

  void OnTick(const TickData& tick);

  void OnAccount(const Account& account);
  void OnPositions(std::vector<Position>* positions);
  void OnTrades(std::vector<Trade>* trades);
  bool OnTimer();

  void OnSecondaryMarketTraded(const Trade& rsp);  // 二级市场买卖

  uint64_t next_order_id() { return next_oms_order_id_++; }

 private:
  std::shared_ptr<Gateway> gateway_{nullptr};
  const FlareTraderConfig* config_;

  std::vector<yijinjing::JournalReaderPtr> trade_msg_readers_;
  std::vector<yijinjing::JournalWriterPtr> rsp_writers_;
  std::map<std::string, std::size_t> strategy_name_to_index_;

  std::set<std::string> subscription_set_;
  std::map<uint32_t, std::vector<yijinjing::JournalWriterPtr>> md_dispatch_map_;

  volatile bool is_logon_{false};
  uint64_t next_oms_order_id_{1};

  SpinLock spinlock_;
  Account account_;
  PositionCalculator pos_calculator_;
  RedisPositionSetter redis_pos_updater_;
  OrderMap order_map_;
  std::unique_ptr<RiskManagementSystem> rms_{nullptr};
  TimerThread timer_thread_;
};

}  // namespace ft

#endif  // FT_SRC_TRADER_OMS_H_
