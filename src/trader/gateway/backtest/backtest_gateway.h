// Copyright [2020-2021] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_SRC_GATEWAY_BACKTEST_BACKTEST_GATEWAY_H_
#define FT_SRC_GATEWAY_BACKTEST_BACKTEST_GATEWAY_H_

#include <string>
#include <vector>

#include "trader/gateway/gateway.h"

namespace ft {

class BacktestGateway : public Gateway {
 public:
  bool Init(const GatewayConfig& config) override;

  bool SendOrder(const OrderRequest& order, uint64_t* privdata_ptr) override;
  bool CancelOrder(uint64_t order_id, uint64_t privdata) override;

  bool Subscribe(const std::vector<std::string>& sub_list) override;

  bool QueryPositions() override;
  bool QueryAccount() override;
  bool QueryTrades() override;

  void OnNotify(uint64_t signal) override;

 private:
  bool LoadHistoryData(const std::string& history_data_file);
};

}  // namespace ft

#endif  //  FT_SRC_GATEWAY_BACKTEST_BACKTEST_GATEWAY_H_
