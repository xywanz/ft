// Copyright [2020-2021] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_SRC_GATEWAY_BACK_TEST_BACK_TEST_GATEWAY_H_
#define FT_SRC_GATEWAY_BACK_TEST_BACK_TEST_GATEWAY_H_

#include <condition_variable>
#include <list>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

#include "ft/component/position_calculator.h"
#include "ft/trader/gateway.h"

namespace ft {

class BackTestGateway : public Gateway {
 public:
  bool Init(const GatewayConfig& config) override;
  void Logout() override;

  bool SendOrder(const OrderRequest& order, uint64_t* privdata_ptr) override;
  bool CancelOrder(uint64_t order_id, uint64_t privdata) override;

  bool Subscribe(const std::vector<std::string>& sub_list) override;

  bool QueryPositions() override;
  bool QueryAccount() override;
  bool QueryTrades() override;

  void OnNotify(uint64_t signal) override;

  void operator()(const OrderAcceptance& acceptance) { OnOrderAccepted(acceptance); }
  void operator()(const OrderRejection& rejection) { OnOrderRejected(rejection); }
  void operator()(const Trade& trade) { OnOrderTraded(trade); }
  void operator()(const OrderCancellation& cancellation) { OnOrderCanceled(cancellation); }
  void operator()(const OrderCancelRejection& cxl_rejection) {
    OnOrderCancelRejected(cxl_rejection);
  }
  void operator()(const TickData& tick) {
    current_ticks_[tick.ticker_id] = tick;
    MatchOrders(tick);
    OnTick(tick);
  }

 private:
  bool LoadHistoryData(const std::string& history_data_file);

  bool CheckOrder(const OrderRequest& order) const;
  bool CheckAndUpdateContext(const OrderRequest& order);

  void UpdateTraded(const OrderRequest& order, const TickData& tick);
  void UpdateCanceled(const OrderRequest& order);
  void UpdatePnl(const TickData& tick);
  void UpdateAccount(const TickData& tick);
  void UpdateAccount();

  bool MatchOrder(const OrderRequest& order, const TickData& tick);
  void MatchOrders(const TickData& tick);

  void BackgroudTask();

  const TickData& get_current_tick(uint32_t ticker_id) const {
    return current_ticks_.at(ticker_id);
  }

 private:
  struct BackTestContext {
    Account account{};
    PositionCalculator pos_calculator{};
    std::unordered_map<uint64_t, std::list<OrderRequest>> pending_orders;
  };

  using Message = std::variant<OrderAcceptance, OrderRejection, Trade, OrderCancellation,
                               OrderCancelRejection, TickData>;

 private:
  BackTestContext ctx_;
  std::unordered_map<uint32_t, TickData> current_ticks_;
  std::queue<Message> msg_queue_;
  std::mutex mutex_;
  std::condition_variable cv_;

  std::vector<TickData> history_data_;
  std::size_t current_tick_pos_ = 0;
};

}  // namespace ft

#endif  //  FT_SRC_GATEWAY_BACK_TEST_BACK_TEST_GATEWAY_H_
