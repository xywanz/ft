// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "Gateway/Virtual/VirtualTradeApi.h"

#include <spdlog/spdlog.h>

#include <thread>

#include "Gateway/Virtual/VirtualGateway.h"

namespace ft {

VirtualTradeApi::VirtualTradeApi() {
  auto contract = ContractTable::get_by_ticker("rb2009");
  assert(contract);
  lastest_quotes_[contract->index] = {200, 200};
}

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
    if (quote_iter == lastest_quotes_.end()) continue;
    const auto& quote = quote_iter->second;
    if ((order.direction == Direction::BUY && quote.ask > 0 &&
         order.price >= quote.ask - 1e-5) ||
        (order.direction == Direction::SELL && quote.bid > 0 &&
         order.price <= quote.bid + 1e-5)) {
      gateway_->on_order_traded(order.order_id, order.volume, order.price);
      iter = order_list.erase(iter);
    }
  }
}

void VirtualTradeApi::process_pendings() {
  std::unique_lock<std::mutex> lock(mutex_);
  for (auto pending_iter = pendings_.begin();
       pending_iter != pendings_.end();) {
    auto& order = *pending_iter;
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

void VirtualTradeApi::set_gateway(VirtualGateway* gateway) {
  gateway_ = gateway;
}

void VirtualTradeApi::start() {
  std::thread([this] {
    for (;;) {
      process_pendings();
      __asm__ __volatile__("pause" : : : "memory");
    }
  }).detach();
}

}  // namespace ft
