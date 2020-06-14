// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "gateway/virtual/virtual_api.h"

#include <spdlog/spdlog.h>

#include <thread>

#include "gateway/virtual/random_walk.h"
#include "gateway/virtual/virtual_gateway.h"
#include "utils/misc.h"

namespace ft {

VirtualApi::VirtualApi() {}

bool VirtualApi::insert_order(VirtualOrderReq* req) {
  if (req->volume <= 0) {
    spdlog::error("[VirtualApi::insert_order] Invalid volume");
    return false;
  }

  if (req->type == OrderType::MARKET || req->type == OrderType::BEST) {
    spdlog::error("[VirtualApi::insert_order] unsupported order type");
    return false;
  }

  if (req->price < 1e-5) {
    spdlog::error("[VirtualApi::insert_order] Invalid price");
    return false;
  }

  if (req->direction != Direction::BUY && req->direction != Direction::SELL) {
    spdlog::error("[VirtualApi::insert_order] unsupported direction");
    return false;
  }

  auto contract = ContractTable::get_by_index(req->ticker_index);
  if (!contract) {
    spdlog::error("[VirtualApi::insert_order] Contract not found");
    return false;
  }

  std::unique_lock<std::mutex> lock(mutex_);
  pendings_.emplace_back(*req);
  lock.unlock();

  cv_.notify_one();
  return true;
}

void VirtualApi::update_quote(uint32_t ticker_index, double ask, double bid) {
  std::unique_lock<std::mutex> lock(mutex_);
  auto& quote = lastest_quotes_[ticker_index];
  quote.ask = ask;
  quote.bid = bid;

  auto& order_list = limit_orders_[ticker_index];
  for (auto iter = order_list.begin(); iter != order_list.end();) {
    auto& order = *iter;

    auto quote_iter = lastest_quotes_.find(ticker_index);
    if (quote_iter == lastest_quotes_.end()) {
      ++iter;
      continue;
    }

    const auto& quote = quote_iter->second;
    if ((order.direction == Direction::BUY && quote.ask > 0 &&
         order.price >= quote.ask - 1e-5) ||
        (order.direction == Direction::SELL && quote.bid > 0 &&
         order.price <= quote.bid + 1e-5)) {
      gateway_->on_order_traded(order.engine_order_id, order.volume,
                                order.price);
      iter = order_list.erase(iter);
    } else {
      ++iter;
    }
  }
}

bool VirtualApi::cancel_order(uint64_t engine_order_id) {
  std::unique_lock<std::mutex> lock(mutex_);
  for (auto& [ticker_index, order_list] : limit_orders_) {
    UNUSED(ticker_index);
    for (auto iter = order_list.begin(); iter != order_list.end(); ++iter) {
      auto& order = *iter;
      if (engine_order_id == order.engine_order_id) {
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
        gateway_->on_order_canceled(order.engine_order_id, order.volume);
        pending_iter = pendings_.erase(pending_iter);
        continue;
      }

      gateway_->on_order_accepted(order.engine_order_id);

      auto iter = lastest_quotes_.find(order.ticker_index);
      if (iter == lastest_quotes_.end()) {
        if (order.type == OrderType::FAK || order.type == OrderType::FOK) {
          gateway_->on_order_canceled(order.engine_order_id, order.volume);
          pending_iter = pendings_.erase(pending_iter);
          continue;
        }
      }

      const auto& quote = iter->second;
      if ((order.direction == Direction::BUY && quote.ask > 0 &&
           order.price >= quote.ask - 1e-5) ||
          (order.direction == Direction::SELL && quote.bid > 0 &&
           order.price <= quote.bid + 1e-5)) {
        gateway_->on_order_traded(order.engine_order_id, order.volume,
                                  quote.ask);
      } else if (order.type == OrderType::LIMIT) {
        limit_orders_[order.ticker_index].emplace_back(order);
      } else {
        gateway_->on_order_canceled(order.engine_order_id, order.volume);
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
    tick.ticker_index = contract->index;
    tick.ask[0] = walker.next();
    tick.bid[0] = tick.ask[0] - 1;
    tick.last_price = (random() & 0xf) >= 8 ? tick.ask[0] : tick.bid[0];

    update_quote(tick.ticker_index, tick.ask[0], tick.bid[0]);
    gateway_->on_tick(&tick);
    std::this_thread::sleep_for(std::chrono::milliseconds(15));
  }
}

}  // namespace ft
