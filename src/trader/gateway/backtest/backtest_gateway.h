// Copyright [2020-present] <Copyright Kevin, kevin.lau.gd@gmail.com>

#pragma once

#include <memory>
#include <string>
#include <vector>

#include "trader/gateway/backtest/match_engine/match_engine.h"
#include "trader/gateway/gateway.h"

namespace ft {

// TODO(K): 资金账户计算
class BacktestGateway : public Gateway, public OrderEventListener {
 public:
  bool Init(const GatewayConfig& config) override;

  bool SendOrder(const OrderRequest& order, uint64_t* privdata_ptr) override;
  bool CancelOrder(uint64_t order_id, uint64_t privdata) override;

  bool Subscribe(const std::vector<std::string>& sub_list) override;

  bool QueryPositions() override;
  bool QueryAccount() override;
  bool QueryTrades() override;
  bool QueryOrders() override;

  void OnNotify(uint64_t signal) override;

  void OnAccepted(OrderAcceptedRsp* rsp) override;
  void OnRejected(OrderRejectedRsp* rsp) override;
  void OnTraded(OrderTradedRsp* rsp) override;
  void OnCanceled(OrderCanceledRsp* rsp) override;
  void OnCancelRejected(OrderCancelRejectedRsp* rsp) override;

 private:
  bool LoadHistoryData(const std::string& history_data_file);

 private:
  std::unique_ptr<MatchEngine> match_engine_;
  std::vector<TickData> history_data_;
  uint64_t tick_cursor_ = 0;
};

}  // namespace ft
