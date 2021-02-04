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

#include "gateway/gateway.h"
#include "utils/portfolio.h"

namespace ft {

class BackTestGateway : public Gateway {
 public:
  bool Login(BaseOrderManagementSystem* oms, const Config& config) override;
  void Logout() override;

  bool SendOrder(const OrderRequest& order, uint64_t* privdata_ptr) override;
  bool CancelOrder(uint64_t order_id, uint64_t privdata) override;

  bool Subscribe(const std::vector<std::string>& sub_list) override;

  bool QueryPosition(const std::string& ticker, Position* result) override;
  bool QueryPositionList(std::vector<Position>* result) override;
  bool QueryAccount(Account* result) override;
  bool QueryTradeList(std::vector<Trade>* result) override;

  void OnNotify(uint64_t signal) override;

  void operator()(OrderAcceptance& acceptance) { oms_->OnOrderAccepted(&acceptance); }
  void operator()(OrderRejection& rejection) { oms_->OnOrderRejected(&rejection); }
  void operator()(Trade& trade) { oms_->OnOrderTraded(&trade); }
  void operator()(OrderCancellation& cancellation) { oms_->OnOrderCanceled(&cancellation); }
  void operator()(OrderCancelRejection& cxl_rejection) {
    oms_->OnOrderCancelRejected(&cxl_rejection);
  }
  void operator()(TickData& tick) {
    current_ticks_[tick.ticker_id] = tick;
    MatchOrders(tick);
    oms_->OnTick(&tick);
  }

 private:
  bool CheckOrder(const OrderRequest& order) const;
  bool CheckAndUpdateContext(const OrderRequest& order);

  bool MatchOrder(const OrderRequest& order, const TickData& tick);
  void MatchOrders(const TickData& tick);

  void UpdateTraded(const OrderRequest& order, const TickData& tick);
  void UpdateCanceled(const OrderRequest& order);

  void BackgroudTask();

  const TickData& get_current_tick(uint32_t ticker_id) const {
    return current_ticks_.at(ticker_id);
  }

 private:
  struct BackTestContext {
    Account account{};
    Portfolio portfolio{};
    std::unordered_map<uint64_t, std::list<OrderRequest>> pending_orders;
  };

  using Message = std::variant<OrderAcceptance, OrderRejection, Trade, OrderCancellation,
                               OrderCancelRejection, TickData>;

 private:
  BaseOrderManagementSystem* oms_ = nullptr;
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
