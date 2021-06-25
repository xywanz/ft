// Copyright [2020-present] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "trader/gateway/backtest/match_engine/simple_match_engine.h"

#include "ft/base/contract_table.h"
#include "ft/utils/misc.h"

namespace ft {

bool SimpleMatchEngine::Init() {
  orders_.resize(ContractTable::size() + 1);
  ticks_.resize(ContractTable::size() + 1);
  return true;
}

bool SimpleMatchEngine::InsertOrder(const OrderRequest& order) {
  auto& tick = ticks_[order.contract->ticker_id];
  double ask = tick.ask[0];
  double bid = tick.bid[0];

  switch (order.type) {
    case OrderType::kLimit: {
      if ((order.direction == Direction::kBuy && tick.ask[0] > 0 && order.price >= tick.ask[0]) ||
          (order.direction == Direction::kSell && tick.bid[0] > 0 && order.price <= tick.bid[0])) {
        double price = order.direction == Direction::kBuy ? ask : bid;
        listener()->OnTraded(order, order.volume, price, cur_timestamp_us_);
      } else {
        orders_[order.contract->ticker_id].emplace(order.order_id, order);
        listener()->OnAccepted(order);
      }
      break;
    }
    case OrderType::kBest: {
      if ((order.direction == Direction::kBuy && IsEqual(bid, 0.0)) ||
          (order.direction == Direction::kSell && IsEqual(ask, 0.0))) {
        OrderRejectedRsp rsp{order.order_id};
        listener()->OnRejected(order);
      } else {
        OrderRequest order_to_insert = order;
        order_to_insert.price = order.direction == Direction::kBuy ? bid : ask;
        orders_[order.contract->ticker_id].emplace(order.order_id, order_to_insert);
        listener()->OnAccepted(order);
      }
      break;
    }
    case OrderType::kMarket: {
      double price = order.direction == Direction::kBuy ? ask : bid;
      if (IsEqual(price, 0.0)) {
        listener()->OnCanceled(order, order.volume);
      } else {
        listener()->OnTraded(order, order.volume, price, cur_timestamp_us_);
      }
      break;
    }
    case OrderType::kFak:
    case OrderType::kFok: {
      if ((order.direction == Direction::kBuy && tick.ask[0] > 0 && order.price >= tick.ask[0]) ||
          (order.direction == Direction::kSell && tick.bid[0] > 0 && order.price <= tick.bid[0])) {
        double price = order.direction == Direction::kBuy ? ask : bid;
        listener()->OnTraded(order, order.volume, price, cur_timestamp_us_);
      } else {
        listener()->OnCanceled(order, order.volume);
      }
      break;
    }
    default: {
      return false;
    }
  }

  return true;
}

bool SimpleMatchEngine::CancelOrder(uint64_t order_id, uint32_t ticker_id) {
  auto& map = orders_[ticker_id];
  auto it = map.find(order_id);
  if (it == map.end()) {
    listener()->OnCancelRejected(order_id);
  } else {
    auto& order = it->second;
    listener()->OnCanceled(order, order.volume);
    map.erase(it);
  }
  return true;
}

void SimpleMatchEngine::OnNewTick(const TickData& tick) {
  cur_timestamp_us_ = tick.exchange_timestamp_us;
  ticks_[tick.ticker_id] = tick;

  auto& map = orders_[tick.ticker_id];
  for (auto it = map.begin(); it != map.end();) {
    auto& order = it->second;
    if ((order.direction == Direction::kBuy && tick.ask[0] > 0 && order.price >= tick.ask[0]) ||
        (order.direction == Direction::kSell && tick.bid[0] > 0 && order.price <= tick.bid[0])) {
      listener()->OnTraded(order, order.volume, order.price, cur_timestamp_us_);
      it = map.erase(it);
    } else {
      ++it;
    }
  }
}

}  // namespace ft
