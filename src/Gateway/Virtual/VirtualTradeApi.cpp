// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "Gateway/Virtual/VirtualTradeApi.h"

#include <spdlog/spdlog.h>

#include <thread>

#include "Gateway/Virtual/VirtualGateway.h"

namespace ft {

VirtualTradeApi::VirtualTradeApi() {}

uint64_t VirtualTradeApi::insert_order(VirtualOrderReq* req) {
  if (req->volume <= 0) {
    spdlog::error("[VirtualTradeApi::insert_order] Invalid volume");
    return 0;
  }

  if (req->type == OrderType::MARKET || req->type == OrderType::BEST) {
    spdlog::error("[VirtualTradeApi::insert_order] unsupported order type");
    return 0;
  }

  if (req->price < 1e-5) {
    spdlog::error("[VirtualTradeApi::insert_order] Invalid price");
    return 0;
  }

  if (req->direction != Direction::BUY && req->direction != Direction::SELL) {
    spdlog::error("[VirtualTradeApi::insert_order] unsupported direction");
    return false;
  }

  auto contract = ContractTable::get_by_index(req->ticker_index);
  if (!contract) {
    spdlog::error("[VirtualTradeApi::insert_order] Contract not found");
    return 0;
  }

  req->order_id = next_order_id_++;
  std::unique_lock<std::mutex> lock(mutex_);
  pendings_.emplace_back(*req);
  lock.unlock();
  cv_.notify_one();
  return req->order_id;
}

void VirtualTradeApi::update_quote(uint32_t ticker_index, double ask,
                                   double bid) {
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
      gateway_->on_order_traded(order.order_id, order.volume, order.price);
      iter = order_list.erase(iter);
    } else {
      ++iter;
    }
  }
}

void VirtualTradeApi::process_pendings() {
  std::unique_lock<std::mutex> lock(mutex_);
  cv_.wait(lock, [this]() { return !pendings_.empty(); });

  for (auto pending_iter = pendings_.begin();
       pending_iter != pendings_.end();) {
    auto& order = *pending_iter;
    if (order.to_canceled) {
      gateway_->on_order_canceled(order.order_id, order.volume);
      pending_iter = pendings_.erase(pending_iter);
      continue;
    }

    gateway_->on_order_accepted(order.order_id);

    auto iter = lastest_quotes_.find(order.ticker_index);
    if (iter == lastest_quotes_.end()) {
      if (order.type == OrderType::FAK || order.type == OrderType::FOK) {
        gateway_->on_order_canceled(order.order_id, order.volume);
        pending_iter = pendings_.erase(pending_iter);
        continue;
      }
    }

    const auto& quote = iter->second;
    if ((order.direction == Direction::BUY && quote.ask > 0 &&
         order.price >= quote.ask - 1e-5) ||
        (order.direction == Direction::SELL && quote.bid > 0 &&
         order.price <= quote.bid + 1e-5)) {
      gateway_->on_order_traded(order.order_id, order.volume, quote.ask);
    } else if (order.direction == Direction::SELL) {
      limit_orders_[order.ticker_index].emplace_back(order);
    } else {
      gateway_->on_order_canceled(order.order_id, order.volume);
    }

    pending_iter = pendings_.erase(pending_iter);
  }
}

bool VirtualTradeApi::cancel_order(uint64_t order_id) {
  std::unique_lock<std::mutex> lock(mutex_);
  for (auto& [ticker_index, order_list] : limit_orders_) {
    for (auto iter = order_list.begin(); iter != order_list.end(); ++iter) {
      auto& order = *iter;
      if (order_id == order.order_id) {
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

void VirtualTradeApi::set_spi(VirtualGateway* gateway) {
  gateway_ = gateway;
}

void VirtualTradeApi::start() {
  std::thread([this] {
    auto contract = ContractTable::get_by_ticker("rb2009");
    assert(contract);
    for (;;) {
      TickData tick{};
      tick.ticker_index = contract->index;
      tick.ask[0] = 200;
      tick.bid[0] = 200;

      update_quote(tick.ticker_index, tick.ask[0], tick.bid[0]);
      gateway_->on_tick(&tick);
      std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
  }).detach();

  std::thread([this] {
    for (;;) process_pendings();
  }).detach();
}

}  // namespace ft
