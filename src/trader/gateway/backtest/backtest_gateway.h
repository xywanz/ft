// Copyright [2020-present] <Copyright Kevin, kevin.lau.gd@gmail.com>

#pragma once

#include <memory>
#include <string>
#include <vector>

#include "ft/component/position/calculator.h"
#include "ft/utils/spinlock.h"
#include "trader/gateway/backtest/fund_calculator.h"
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

  void OnAccepted(const OrderRequest& order) override;
  void OnRejected(const OrderRequest& order) override;
  void OnTraded(const OrderRequest& order, int volume, double price,
                uint64_t timestamp_us) override;
  void OnCanceled(const OrderRequest& order, int canceled_volume) override;
  void OnCancelRejected(uint64_t order_id) override;

 private:
  bool LoadHistoryData(const std::string& history_data_file);

  bool CheckOrder(const OrderRequest& order) const;

 private:
  std::shared_ptr<MatchEngine> match_engine_;
  PositionCalculator pos_calculator_;
  FundCalculator fund_calculator_;
  std::vector<TickData> current_ticks_;
  SpinLock spinlock_;

  std::vector<TickData> history_data_;
  uint64_t tick_cursor_ = 0;
};

}  // namespace ft
