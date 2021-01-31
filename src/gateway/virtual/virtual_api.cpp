// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "virtual_api.h"

#include <spdlog/spdlog.h>

#include <thread>

#include "gateway/virtual/virtual_gateway.h"
#include "random_walk.h"
#include "utils/misc.h"

namespace ft {

VirtualApi::VirtualApi() {
  account_.account_id = 1234;
  account_.total_asset = account_.cash = 100000000;
  positions_.Init(ContractTable::size());
}

bool VirtualApi::InsertOrder(VirtualOrderRequest* req) {
  if (!CheckOrder(req)) return false;
  if (!CheckAndUpdatePosAccount(req)) return false;

  std::unique_lock<std::mutex> lock(mutex_);
  pendings_.emplace_back(*req);
  lock.unlock();

  cv_.notify_one();
  return true;
}

void VirtualApi::UpdateQuote(uint32_t ticker_id, double ask, double bid) {
  std::unique_lock<std::mutex> lock(mutex_);
  auto& quote = lastest_quotes_[ticker_id];
  quote.ask = ask;
  quote.bid = bid;

  auto& order_list = limit_orders_[ticker_id];
  for (auto iter = order_list.begin(); iter != order_list.end();) {
    auto& order = *iter;

    auto quote_iter = lastest_quotes_.find(ticker_id);
    if (quote_iter == lastest_quotes_.end()) {
      ++iter;
      continue;
    }

    const auto& quote = quote_iter->second;
    if ((order.direction == Direction::kBuy && quote.ask > 0 && order.price >= quote.ask - 1e-5) ||
        (order.direction == Direction::kSell && quote.bid > 0 && order.price <= quote.bid + 1e-5)) {
      UpdateTraded(order, quote);
      iter = order_list.erase(iter);
    } else {
      ++iter;
    }
  }
}

bool VirtualApi::CancelOrder(uint64_t oms_order_id) {
  std::unique_lock<std::mutex> lock(mutex_);
  for (auto& [ticker_id, order_list] : limit_orders_) {
    UNUSED(ticker_id);
    for (auto iter = order_list.begin(); iter != order_list.end(); ++iter) {
      auto& order = *iter;
      if (oms_order_id == order.oms_order_id) {
        order.to_canceled = true;
        pendings_.emplace_back(order);
        order_list.erase(iter);
        lock.unlock();
        cv_.notify_one();
        return true;
      }
    }
  }

  return false;
}

bool VirtualApi::QueryAccount(Account* result) {
  std::unique_lock<std::mutex> lock(mutex_);
  *result = account_;
  return true;
}

void VirtualApi::set_spi(VirtualGateway* gateway) { gateway_ = gateway; }

void VirtualApi::StartQuoteServer() {
  std::thread([this] { DisseminateMarketData(); }).detach();
}

void VirtualApi::StartTradeServer() {
  std::thread([this] { process_pendings(); }).detach();
}

void VirtualApi::process_pendings() {
  std::unique_lock<std::mutex> lock(mutex_);

  for (;;) {
    cv_.wait(lock, [this]() { return !pendings_.empty(); });

    for (auto pending_iter = pendings_.begin(); pending_iter != pendings_.end();) {
      auto& order = *pending_iter;
      if (order.to_canceled) {
        UpdateCanceled(order);
        pending_iter = pendings_.erase(pending_iter);
        continue;
      }

      gateway_->OnOrderAccepted(order.oms_order_id);

      auto iter = lastest_quotes_.find(order.ticker_id);
      if (iter == lastest_quotes_.end()) {
        if (order.type == OrderType::kFak || order.type == OrderType::FOK) {
          UpdateCanceled(order);
          pending_iter = pendings_.erase(pending_iter);
          continue;
        }
      }

      const auto& quote = iter->second;
      if ((order.direction == Direction::kBuy && quote.ask > 0 &&
           order.price >= quote.ask - 1e-5) ||
          (order.direction == Direction::kSell && quote.bid > 0 &&
           order.price <= quote.bid + 1e-5)) {
        UpdateTraded(order, quote);
      } else if (order.type == OrderType::kLimit) {
        limit_orders_[order.ticker_id].emplace_back(order);
      } else {
        UpdateCanceled(order);
      }

      pending_iter = pendings_.erase(pending_iter);
    }
  }
}

void VirtualApi::DisseminateMarketData() {
  auto contract = ContractTable::get_by_ticker("rb2011");
  assert(contract);

  RandomWalk walker(10000, 1);

  for (;;) {
    TickData tick{};
    tick.ticker_id = contract->ticker_id;
    tick.ask[0] = walker.next();
    tick.bid[0] = tick.ask[0] - 2;
    tick.last_price = (random() & 0xf) >= 8 ? tick.ask[0] : tick.bid[0];

    UpdateQuote(tick.ticker_id, tick.ask[0], tick.bid[0]);
    gateway_->OnTick(&tick);
    std::this_thread::sleep_for(std::chrono::milliseconds(15));
  }
}

bool VirtualApi::CheckOrder(const VirtualOrderRequest* req) const {
  if (req->volume <= 0) {
    spdlog::error("[VirtualApi::CheckOrder] Invalid volume");
    return false;
  }
  if (req->type == OrderType::MARKET || req->type == OrderType::BEST) {
    spdlog::error("[VirtualApi::CheckOrder] unsupported order type");
    return false;
  }
  if (req->price < 1e-5) {
    spdlog::error("[VirtualApi::CheckOrder] Invalid price");
    return false;
  }
  if (req->direction != Direction::kBuy && req->direction != Direction::kSell) {
    spdlog::error("[VirtualApi::CheckOrder] unsupported direction");
    return false;
  }
  if (!ContractTable::get_by_index(req->ticker_id)) {
    spdlog::error("[VirtualApi::CheckOrder] Contract not found");
    return false;
  }
  return true;
}

bool VirtualApi::CheckAndUpdatePosAccount(const VirtualOrderRequest* req) {
  std::unique_lock<std::mutex> lock(mutex_);
  if (req->offset == Offset::kOpen) {
    double fund_needed = req->price * req->volume;
    if (account_.cash < fund_needed) return false;
    account_.cash -= fund_needed;
    account_.frozen += fund_needed;
  } else {
    const auto* pos = positions_.get_position(req->ticker_id);
    const auto& detail = req->direction == Direction::kSell ? pos->long_pos : pos->short_pos;
    if (detail.holdings - detail.close_pending < req->volume) return false;
  }

  positions_.UpdatePending(req->ticker_id, req->direction, req->offset, req->volume);
  return true;
}

void VirtualApi::UpdateCanceled(const VirtualOrderRequest& order) {
  if (order.offset == Offset::kOpen) {
    double fund_returned = order.volume * order.price;
    account_.frozen -= fund_returned;
    account_.cash += fund_returned;
  }
  positions_.UpdatePending(order.ticker_id, order.direction, order.offset, 0 - order.volume);
  gateway_->OnOrderCanceled(order.oms_order_id, order.volume);
}

void VirtualApi::UpdateTraded(const VirtualOrderRequest& order, const LatestQuote& quote) {
  double price = order.direction == Direction::kBuy ? quote.ask : quote.bid;
  if (order.offset == Offset::kOpen) {
    double fund_returned = order.volume * order.price;
    double cost = order.volume * price;
    account_.frozen -= fund_returned;
    account_.cash += fund_returned;
    // account_.margin += cost;
    account_.cash -= cost;
  } else {
    double fund_returned = order.volume * price;
    // account_.margin -= fund_returned;
    account_.cash += fund_returned;
  }
  positions_.UpdateTraded(order.ticker_id, order.direction, order.offset, order.volume, quote.ask);
  gateway_->OnOrderTraded(order.oms_order_id, order.volume, quote.ask);
}

}  // namespace ft
