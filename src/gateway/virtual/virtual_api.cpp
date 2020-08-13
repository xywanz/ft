// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "virtual_api.h"

#include <spdlog/spdlog.h>

#include <thread>

#include "random_walk.h"
#include "utils/misc.h"
#include "virtual_gateway.h"

namespace ft {

VirtualApi::VirtualApi() {
  account_.account_id = 1234;
  account_.total_asset = account_.cash = 100000000;
  positions_.init(ContractTable::size());
}

bool VirtualApi::insert_order(VirtualOrderRequest* req) {
  if (!check_order(req)) return false;
  if (!check_and_update_pos_account(req)) return false;

  std::unique_lock<std::mutex> lock(mutex_);
  pendings_.emplace_back(*req);
  lock.unlock();

  cv_.notify_one();
  return true;
}

void VirtualApi::update_quote(uint32_t tid, double ask, double bid) {
  std::unique_lock<std::mutex> lock(mutex_);
  auto& quote = lastest_quotes_[tid];
  quote.ask = ask;
  quote.bid = bid;

  auto& order_list = limit_orders_[tid];
  for (auto iter = order_list.begin(); iter != order_list.end();) {
    auto& order = *iter;

    auto quote_iter = lastest_quotes_.find(tid);
    if (quote_iter == lastest_quotes_.end()) {
      ++iter;
      continue;
    }

    const auto& quote = quote_iter->second;
    if ((order.direction == Direction::BUY && quote.ask > 0 &&
         order.price >= quote.ask - 1e-5) ||
        (order.direction == Direction::SELL && quote.bid > 0 &&
         order.price <= quote.bid + 1e-5)) {
      update_traded(order, quote);
      iter = order_list.erase(iter);
    } else {
      ++iter;
    }
  }
}

bool VirtualApi::cancel_order(uint64_t oms_order_id) {
  std::unique_lock<std::mutex> lock(mutex_);
  for (auto& [tid, order_list] : limit_orders_) {
    UNUSED(tid);
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

bool VirtualApi::query_account(Account* result) {
  std::unique_lock<std::mutex> lock(mutex_);
  *result = account_;
  return true;
}

void VirtualApi::set_spi(VirtualGateway* gateway) { gateway_ = gateway; }

void VirtualApi::start_quote_server() {
  std::thread([this] { disseminate_market_data(); }).detach();
}

void VirtualApi::start_trade_server() {
  std::thread([this] { process_pendings(); }).detach();
}

void VirtualApi::process_pendings() {
  std::unique_lock<std::mutex> lock(mutex_);

  for (;;) {
    cv_.wait(lock, [this]() { return !pendings_.empty(); });

    for (auto pending_iter = pendings_.begin();
         pending_iter != pendings_.end();) {
      auto& order = *pending_iter;
      if (order.to_canceled) {
        update_canceled(order);
        pending_iter = pendings_.erase(pending_iter);
        continue;
      }

      gateway_->on_order_accepted(order.oms_order_id);

      auto iter = lastest_quotes_.find(order.tid);
      if (iter == lastest_quotes_.end()) {
        if (order.type == OrderType::FAK || order.type == OrderType::FOK) {
          update_canceled(order);
          pending_iter = pendings_.erase(pending_iter);
          continue;
        }
      }

      const auto& quote = iter->second;
      if ((order.direction == Direction::BUY && quote.ask > 0 &&
           order.price >= quote.ask - 1e-5) ||
          (order.direction == Direction::SELL && quote.bid > 0 &&
           order.price <= quote.bid + 1e-5)) {
        update_traded(order, quote);
      } else if (order.type == OrderType::LIMIT) {
        limit_orders_[order.tid].emplace_back(order);
      } else {
        update_canceled(order);
      }

      pending_iter = pendings_.erase(pending_iter);
    }
  }
}

void VirtualApi::disseminate_market_data() {
  auto contract = ContractTable::get_by_ticker("rb2009");
  assert(contract);

  RandomWalk walker(10000, 1);

  for (;;) {
    TickData tick{};
    tick.tid = contract->tid;
    tick.ask[0] = walker.next();
    tick.bid[0] = tick.ask[0] - 2;
    tick.last_price = (random() & 0xf) >= 8 ? tick.ask[0] : tick.bid[0];

    update_quote(tick.tid, tick.ask[0], tick.bid[0]);
    gateway_->on_tick(&tick);
    std::this_thread::sleep_for(std::chrono::milliseconds(15));
  }
}

bool VirtualApi::check_order(const VirtualOrderRequest* req) const {
  if (req->volume <= 0) {
    spdlog::error("[VirtualApi::check_order] Invalid volume");
    return false;
  }
  if (req->type == OrderType::MARKET || req->type == OrderType::BEST) {
    spdlog::error("[VirtualApi::check_order] unsupported order type");
    return false;
  }
  if (req->price < 1e-5) {
    spdlog::error("[VirtualApi::check_order] Invalid price");
    return false;
  }
  if (req->direction != Direction::BUY && req->direction != Direction::SELL) {
    spdlog::error("[VirtualApi::check_order] unsupported direction");
    return false;
  }
  if (!ContractTable::get_by_index(req->tid)) {
    spdlog::error("[VirtualApi::check_order] Contract not found");
    return false;
  }
  return true;
}

bool VirtualApi::check_and_update_pos_account(const VirtualOrderRequest* req) {
  std::unique_lock<std::mutex> lock(mutex_);
  if (req->offset == Offset::OPEN) {
    double fund_needed = req->price * req->volume;
    if (account_.cash < fund_needed) return false;
    account_.cash -= fund_needed;
    account_.frozen += fund_needed;
  } else {
    const auto* pos = positions_.get_position(req->tid);
    const auto& detail =
        req->direction == Direction::SELL ? pos->long_pos : pos->short_pos;
    if (detail.holdings - detail.close_pending < req->volume) return false;
  }

  positions_.update_pending(req->tid, req->direction, req->offset, req->volume);
  return true;
}

void VirtualApi::update_canceled(const VirtualOrderRequest& order) {
  if (order.offset == Offset::OPEN) {
    double fund_returned = order.volume * order.price;
    account_.frozen -= fund_returned;
    account_.cash += fund_returned;
  }
  positions_.update_pending(order.tid, order.direction, order.offset,
                            0 - order.volume);
  gateway_->on_order_canceled(order.oms_order_id, order.volume);
}

void VirtualApi::update_traded(const VirtualOrderRequest& order,
                               const LatestQuote& quote) {
  double price = order.direction == Direction::BUY ? quote.ask : quote.bid;
  if (order.offset == Offset::OPEN) {
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
  positions_.update_traded(order.tid, order.direction, order.offset,
                           order.volume, quote.ask);
  gateway_->on_order_traded(order.oms_order_id, order.volume, quote.ask);
}

}  // namespace ft
