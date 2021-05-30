// Copyright [2020-present] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_SRC_TRADER_GATEWAY_STUB_STUB_GATEWAY_H_
#define FT_SRC_TRADER_GATEWAY_STUB_STUB_GATEWAY_H_

#include <atomic>
#include <string>
#include <thread>
#include <vector>

#include "trader/gateway/gateway.h"

namespace ft {

class StubGateway : public Gateway {
 public:
  bool Init(const GatewayConfig& config) override;

  void Logout() override;

  bool SendOrder(const OrderRequest& order, uint64_t* privdata_ptr) override;

  bool CancelOrder(uint64_t order_id, uint64_t privdata) override;

  bool Subscribe(const std::vector<std::string>& sub_list) override;

  bool QueryPositions() override;

  bool QueryAccount() override;

  bool QueryTrades() override;

 private:
  void GenerateTickData();

 private:
  std::atomic<bool> running_ = false;
  std::vector<std::string> sub_list_;
  std::thread tick_thread_;
};

}  // namespace ft

#endif  // FT_SRC_TRADER_GATEWAY_STUB_STUB_GATEWAY_H_
