// Copyright [2020-present] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "stub_gateway.h"

#include "ft/base/contract_table.h"
#include "ft/base/log.h"

namespace ft {

bool StubGateway::Init(const GatewayConfig& config) { return true; }

void StubGateway::Logout() {
  running_ = false;
  if (tick_thread_.joinable()) {
    tick_thread_.join();
  }
}

bool StubGateway::SendOrder(const OrderRequest& order, uint64_t* privdata_ptr) {
  OnOrderAccepted(OrderAcceptedRsp{order.order_id});

  OrderTradedRsp trade{};
  trade.order_id = order.order_id;
  trade.price = order.price;
  trade.volume = order.volume;

  timespec ts;
  clock_gettime(CLOCK_REALTIME, &ts);
  trade.timestamp_us = ts.tv_nsec / 1000UL + ts.tv_sec * 1000000UL;
  OnOrderTraded(trade);

  return true;
}

bool StubGateway::CancelOrder(uint64_t order_id, uint64_t privdata) {
  OnOrderCanceled(OrderCanceledRsp{order_id});
  return true;
}

bool StubGateway::Subscribe(const std::vector<std::string>& sub_list) {
  sub_list_ = sub_list;
  running_ = true;
  tick_thread_ = std::thread(std::mem_fn(&StubGateway::GenerateTickData), this);
  return true;
}

bool StubGateway::QueryPositions() {
  OnQueryPositionEnd();
  return true;
}

bool StubGateway::QueryAccount() {
  Account account{};
  account.total_asset = account.cash = 10000000;
  OnQueryAccount(account);
  OnQueryAccountEnd();
  return true;
}

bool StubGateway::QueryTrades() {
  OnQueryTradeEnd();
  return true;
}

void StubGateway::GenerateTickData() {
  while (running_) {
    for (auto& ticker : sub_list_) {
      auto* contract = ContractTable::get_by_ticker(ticker);
      if (!contract) {
        LOG_WARN("[StubGateway::GenerateTickData] contract not found. ticker:{}", ticker);
        continue;
      }
      TickData tick{};
      tick.ticker_id = contract->ticker_id;
      tick.ask[0] = 888.0;
      tick.ask_volume[0] = 8;
      tick.bid[0] = 880.0;
      tick.bid_volume[0] = 6;
      tick.last_price = 885.0;

      timespec ts;
      clock_gettime(CLOCK_REALTIME, &ts);
      tick.local_timestamp_us = ts.tv_nsec / 1000UL + ts.tv_sec * 1000000UL;

      OnTick(tick);
      std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
  }
}

}  // namespace ft
