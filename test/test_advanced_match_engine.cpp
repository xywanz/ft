#include <queue>

#include "ft/base/contract_table.h"
#include "gtest/gtest.h"
#include "trader/gateway/backtest/match_engine/advanced_match_engine.h"

using namespace ft;

bool is_contractable_inited = [] {
  std::vector<Contract> contracts;
  contracts.resize(1);
  for (auto& contract : contracts) {
    contract.size = 1;
  }
  return ContractTable::Init(std::move(contracts));
}();

enum EventType {
  ACCEPTED = 100,
  REJECTED,
  TRADED,
  CANCELED,
  CANCEL_REJECTED,
};

struct Response {
  EventType type;
  int volume;
  double price;
};

class TestListener : public OrderEventListener {
 public:
  void OnAccepted(const OrderRequest& order) override {
    Response rsp{};
    rsp.type = ACCEPTED;
    rsp_queue_.push(rsp);
  }

  void OnRejected(const OrderRequest& order) override {
    Response rsp{};
    rsp.type = REJECTED;
    rsp_queue_.push(rsp);
  }

  void OnTraded(const OrderRequest& order, int volume, double price,
                uint64_t timestamp_us) override {
    Response rsp{};
    rsp.type = TRADED;
    rsp.volume = volume;
    rsp.price = price;
    rsp_queue_.push(rsp);
  }

  void OnCanceled(const OrderRequest& order, int canceled_volume) override {
    Response rsp{};
    rsp.type = CANCELED;
    rsp_queue_.push(rsp);
  }

  void OnCancelRejected(uint64_t order_id) override {
    Response rsp{};
    rsp.type = CANCEL_REJECTED;
    rsp_queue_.push(rsp);
  }

  auto& GetRspQueue() { return rsp_queue_; }

 private:
  std::queue<Response> rsp_queue_;
};

TEST(AdvancedMatchEngine, Match) {
  ASSERT_TRUE(is_contractable_inited);
  auto* contract = ContractTable::get_by_index(1);
  ASSERT_TRUE(contract != nullptr);

  AdvancedMatchEngine engine;
  TestListener listener;
  auto& rsp_queue = listener.GetRspQueue();
  engine.RegisterListener(&listener);
  ASSERT_TRUE(engine.Init());

  TickData tick{};
  tick.ticker_id = 1;
  tick.volume = 0;
  tick.turnover = 0;
  tick.ask[0] = 101;
  tick.ask_volume[0] = 20;
  tick.bid[0] = 99;
  tick.bid_volume[0] = 16;
  engine.OnNewTick(tick);
  ASSERT_TRUE(rsp_queue.empty());

  OrderRequest order;
  order.contract = contract;
  order.direction = Direction::kBuy;
  order.offset = Offset::kOpen;
  order.order_id = 1;
  order.type = OrderType::kLimit;
  order.price = 99;
  order.volume = 1;
  engine.InsertOrder(order);
  ASSERT_EQ(rsp_queue.size(), 1);
  auto rsp = rsp_queue.front();
  rsp_queue.pop();
  ASSERT_EQ(rsp.type, ACCEPTED);

  order.contract = contract;
  order.direction = Direction::kBuy;
  order.offset = Offset::kOpen;
  order.type = OrderType::kLimit;
  order.order_id = 1;
  order.price = 101;
  order.volume = 1;
  engine.InsertOrder(order);
  ASSERT_EQ(rsp_queue.size(), 1);
  rsp = rsp_queue.front();
  rsp_queue.pop();
  ASSERT_EQ(rsp.type, TRADED);
  ASSERT_EQ(rsp.volume, 1);
  ASSERT_DOUBLE_EQ(rsp.price, 101);

  // 把前面16手消耗完，此时我们的订单不会发生成交
  tick.ticker_id = 1;
  tick.volume += 16;
  tick.turnover += 16 * 98;
  tick.ask[0] = 101;
  tick.ask_volume[0] = 20;
  tick.bid[0] = 99;
  tick.bid_volume[0] = 16;
  engine.OnNewTick(tick);
  ASSERT_TRUE(rsp_queue.empty());

  tick.ticker_id = 1;
  tick.volume += 1;
  tick.turnover += 1 * 99;
  tick.ask[0] = 101;
  tick.ask_volume[0] = 20;
  tick.ask[1] = 102;
  tick.ask_volume[1] = 20;
  tick.ask[2] = 103;
  tick.ask_volume[2] = 20;
  tick.bid[0] = 99;
  tick.bid_volume[0] = 16;
  // 此时队列位置应该为0，只要在该价位有成交，我们的订单必定成交
  engine.OnNewTick(tick);
  ASSERT_EQ(rsp_queue.size(), 1);
  rsp = rsp_queue.front();
  rsp_queue.pop();
  ASSERT_EQ(rsp.type, TRADED);
  ASSERT_EQ(rsp.volume, 1);
  ASSERT_DOUBLE_EQ(rsp.price, 99);

  // 在每个ask各挂一个订单
  order.direction = Direction::kSell;
  order.offset = Offset::kOpen;
  order.type = OrderType::kLimit;
  order.order_id = 2;
  order.price = 101;
  order.volume = 3;
  engine.InsertOrder(order);

  order.order_id = 3;
  order.price = 102;
  order.volume = 2;
  engine.InsertOrder(order);

  order.order_id = 4;
  order.price = 103;
  order.volume = 1;
  engine.InsertOrder(order);

  // ask成交量为50，只有前面两个订单能够成交
  tick.volume += 50;
  tick.turnover += 50 * 102;
  engine.OnNewTick(tick);
  ASSERT_EQ(rsp_queue.size(), 5);
  rsp = rsp_queue.front();
  rsp_queue.pop();
  ASSERT_EQ(rsp.type, ACCEPTED);
  rsp = rsp_queue.front();
  rsp_queue.pop();
  ASSERT_EQ(rsp.type, ACCEPTED);
  rsp = rsp_queue.front();
  rsp_queue.pop();
  ASSERT_EQ(rsp.type, ACCEPTED);
  rsp = rsp_queue.front();
  rsp_queue.pop();
  ASSERT_EQ(rsp.type, TRADED);
  ASSERT_EQ(rsp.volume, 3);
  ASSERT_DOUBLE_EQ(rsp.price, 101);
  rsp = rsp_queue.front();
  rsp_queue.pop();
  ASSERT_EQ(rsp.type, TRADED);
  ASSERT_EQ(rsp.volume, 2);
  ASSERT_DOUBLE_EQ(rsp.price, 102);
}
