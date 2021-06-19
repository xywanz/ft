// Copyright [2020-present] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "trader/gateway/backtest/match_engine/advanced_match_engine.h"

#include <cassert>

#include "ft/base/contract_table.h"
#include "ft/base/log.h"
#include "ft/utils/misc.h"

namespace ft {

static inline uint64_t u64_price(double p) {
  return (static_cast<uint64_t>(p * 100000000UL) + 50UL) / 10000UL;
}

bool AdvancedMatchEngine::Init() {
  bid_orderbooks_.resize(ContractTable::size());
  ask_orderbooks_.resize(ContractTable::size());
  ticks_.resize(ContractTable::size());
  return true;
}

bool AdvancedMatchEngine::InsertOrder(const OrderRequest& order) {
  auto& tick = ticks_[order.contract->ticker_id - 1];
  double ask = tick.ask[0];
  double bid = tick.bid[0];

  switch (order.type) {
    case OrderType::kLimit: {
      if ((order.direction == Direction::kBuy && tick.ask_volume[0] > 0 && order.price >= ask) ||
          (order.direction == Direction::kSell && tick.bid_volume[0] > 0 && order.price <= bid)) {
        double price = order.direction == Direction::kBuy ? ask : bid;
        listener()->OnTraded(order, order.volume, price, tick.exchange_timestamp_us);
      } else {
        InnerOrder inner_order{};
        inner_order.orig_order = order;
        if (order.direction == Direction::kBuy) {
          for (int level = 0; level < kMaxMarketLevel; ++level) {
            if (IsEqual(tick.bid[level], order.price)) {
              inner_order.queue_position = tick.bid_volume[level];
              LOG_DEBUG("queue_position:{}", inner_order.queue_position);
              break;
            }
          }
          uint64_t price_u64 = u64_price(order.price);
          bid_orderbooks_[order.contract->ticker_id - 1][price_u64].emplace_back(inner_order);
          id_price_map_.emplace(order.order_id, price_u64);
        } else {
          for (int level = 0; level < kMaxMarketLevel; ++level) {
            if (IsEqual(tick.ask[level], order.price)) {
              inner_order.queue_position = tick.ask_volume[level];
              LOG_DEBUG("queue_position:{}", inner_order.queue_position);
              break;
            }
          }
          uint64_t price_u64 = u64_price(order.price);
          ask_orderbooks_[order.contract->ticker_id - 1][price_u64].emplace_back(inner_order);
          id_price_map_.emplace(order.order_id, price_u64);
        }
        listener()->OnAccepted(order);
      }
      break;
    }
    case OrderType::kBest: {
      if ((order.direction == Direction::kBuy && tick.bid_volume[0] == 0) ||
          (order.direction == Direction::kSell && tick.ask_volume[0] == 0)) {
        listener()->OnRejected(order);
      } else {
        InnerOrder inner_order;
        inner_order.orig_order = order;
        if (order.direction == Direction::kBuy) {
          inner_order.orig_order.price = bid;
          inner_order.queue_position = tick.bid_volume[0];
          uint64_t price_u64 = u64_price(bid);
          bid_orderbooks_[order.contract->ticker_id - 1][price_u64].emplace_back(inner_order);
          id_price_map_.emplace(order.order_id, price_u64);
        } else {
          inner_order.orig_order.price = ask;
          inner_order.queue_position = tick.ask_volume[0];
          uint64_t price_u64 = u64_price(ask);
          ask_orderbooks_[order.contract->ticker_id - 1][price_u64].emplace_back(inner_order);
          id_price_map_.emplace(order.order_id, price_u64);
        }
        listener()->OnAccepted(order);
      }
      break;
    }
    case OrderType::kMarket: {
      int volume = order.direction == Direction::kBuy ? tick.ask_volume[0] : tick.bid_volume[0];
      if (volume == 0) {
        listener()->OnCanceled(order, order.volume);
      } else {
        double price = order.direction == Direction::kBuy ? ask : bid;
        listener()->OnTraded(order, order.volume, price, tick.exchange_timestamp_us);
      }
      break;
    }
    case OrderType::kFak:
    case OrderType::kFok: {
      if ((order.direction == Direction::kBuy && tick.ask_volume[0] > 0 && order.price >= ask) ||
          (order.direction == Direction::kSell && tick.bid_volume[0] > 0 && order.price <= bid)) {
        double price = order.direction == Direction::kBuy ? ask : bid;
        listener()->OnTraded(order, order.volume, price, tick.exchange_timestamp_us);
      } else {
        listener()->OnCanceled(order, order.volume);
      }
      break;
    }
    default: { return false; }
  }

  return true;
}

bool AdvancedMatchEngine::CancelOrder(uint64_t order_id, uint32_t ticker_id) {
  auto it = id_price_map_.find(order_id);
  if (it == id_price_map_.end()) {
    return false;
  }
  uint64_t price_u64 = it->second;
  id_price_map_.erase(it);

  auto bid_order_list_it = bid_orderbooks_[ticker_id - 1].find(price_u64);
  if (bid_order_list_it == bid_orderbooks_[ticker_id - 1].end()) {
    auto ask_order_list_it = ask_orderbooks_[ticker_id - 1].find(price_u64);
    if (ask_order_list_it == bid_orderbooks_[ticker_id - 1].end()) {
      // bug
      abort();
    }
    auto& order_list = ask_order_list_it->second;
    for (auto order_it = order_list.begin(); order_it != order_list.end();) {
      if (order_it->orig_order.order_id == order_id) {
        listener()->OnCanceled(order_it->orig_order, order_it->orig_order.volume);
        order_list.erase(order_it);
        if (order_list.empty()) {
          ask_orderbooks_[ticker_id - 1].erase(ask_order_list_it);
        }
        return true;
      }
    }
  } else {
    auto& order_list = bid_order_list_it->second;
    for (auto order_it = order_list.begin(); order_it != order_list.end();) {
      if (order_it->orig_order.order_id == order_id) {
        listener()->OnCanceled(order_it->orig_order, order_it->orig_order.volume);
        order_list.erase(order_it);
        if (order_list.empty()) {
          ask_orderbooks_[ticker_id - 1].erase(bid_order_list_it);
        }
        return true;
      }
    }
  }

  // bug
  abort();
}

void AdvancedMatchEngine::OnNewTick(const TickData& tick) {
  auto& prev_tick = ticks_[tick.ticker_id - 1];
  if (prev_tick.ticker_id == 0) {
    prev_tick = tick;
    return;
  }

  double delta_volume = tick.volume - prev_tick.volume;
  if (delta_volume == 0) {
    ticks_[tick.ticker_id - 1] = tick;
    return;
  }

  // 估算多方和空方的成交量
  // todo: 假如假如多或空没有挂单时，该如何估算双方的成交量
  int bid_filled = 0;
  int ask_filled = 0;
  if (prev_tick.ask_volume[0] == 0 && prev_tick.bid_volume[0] == 0) {
    bid_filled = 0;
    ask_filled = 0;
  } else if (prev_tick.bid_volume[0] == 0) {
    ask_filled = static_cast<int>(delta_volume);
  } else if (prev_tick.ask_volume[0] == 0) {
    bid_filled = static_cast<int>(delta_volume);
  } else {
    // 当多空方都存在时，首先通过两个tick之间的turnover和volume差值，计算出
    // 在这两个tick期间的成交均价avg_price，根据avg_price所处的位置不同，可
    // 以分为三种情况：
    // 1. avg_price >= ask，这种情况下假定所有成交都在空方
    // 2. avg_price <= bid，这种情况下假定所有成交都在多方
    // 3. bid < avg_price < ask，这种情况下，通过如下方法估算双边的成交量
    //    spread = ask - price
    //    bid_percentage = (avg_price - bid) / spread
    //    ask_percentage = (ask - avg_price) / spread
    //    volume = this_tick.volume - prev_tick.volume
    //    bid_filled = bid_percentage * volume
    //    ask_filled = ask_percentage * volume
    double delta_turnover = tick.turnover - prev_tick.turnover;
    auto* contract = ContractTable::get_by_index(tick.ticker_id);
    assert(contract && contract->size > 0);
    double avg_price = delta_turnover / (delta_volume * contract->size);
    if (avg_price >= tick.ask[0]) {
      ask_filled = static_cast<int>(delta_volume);
    } else if (avg_price <= tick.bid[0]) {
      bid_filled = static_cast<int>(delta_volume);
    } else {
      double spread = tick.ask[0] - tick.bid[0];
      assert(spread > 0);
      bid_filled = static_cast<int>(delta_volume * (avg_price - tick.bid[0]) / spread);
      ask_filled = static_cast<int>(delta_volume * (tick.ask[0] - avg_price) / spread);
    }
    LOG_DEBUG("avg_price:{:.4f} bid:{} ask:{}", avg_price, tick.bid[0], tick.ask[0]);
  }

  LOG_DEBUG("bid_filled:{} ask_filled:{}", bid_filled, ask_filled);

  if (bid_filled > 0) {
    auto& orderbook = bid_orderbooks_[tick.ticker_id - 1];

    if (tick.bid_volume[0] > 0) {
      uint64_t bid_u64 = u64_price(tick.bid[0]);
      while (!orderbook.empty()) {
        auto begin = orderbook.begin();
        if (begin->first <= bid_u64) {
          break;
        }
        auto& order_list = begin->second;
        for (auto& order : order_list) {
          listener()->OnTraded(order.orig_order, order.orig_order.volume, order.orig_order.price,
                               tick.exchange_timestamp_us);
          id_price_map_.erase(order.orig_order.order_id);
        }
        orderbook.erase(begin);
      }
    }

    for (int level = 0; level < kMaxMarketLevel; ++level) {
      if (tick.bid_volume[level] == 0) {
        continue;
      }
      auto order_list_it = orderbook.find(u64_price(tick.bid[level]));
      if (order_list_it != orderbook.end()) {
        auto& order_list = order_list_it->second;
        for (auto order_it = order_list.begin(); order_it != order_list.end();) {
          auto& order = *order_it;
          LOG_DEBUG("queue_position:{} bid_filled:{}", order.queue_position, bid_filled);
          if (order.queue_position < bid_filled) {
            listener()->OnTraded(order.orig_order, order.orig_order.volume, order.orig_order.price,
                                 tick.exchange_timestamp_us);
            id_price_map_.erase(order.orig_order.order_id);
            order_it = order_list.erase(order_it);
          } else {
            order.queue_position -= bid_filled;
            ++order_it;
          }
          if (order_list.empty()) {
            orderbook.erase(order_list_it);
          }
        }
      }
      bid_filled -= tick.bid_volume[level];
      if (bid_filled <= 0) {
        break;
      }
    }
  }

  if (ask_filled > 0) {
    auto& orderbook = ask_orderbooks_[tick.ticker_id - 1];

    if (tick.ask_volume[0] > 0) {
      uint64_t ask_u64 = u64_price(tick.bid[0]);
      while (!orderbook.empty()) {
        auto begin = orderbook.begin();
        if (begin->first >= ask_u64) {
          break;
        }
        auto& order_list = begin->second;
        for (auto& order : order_list) {
          listener()->OnTraded(order.orig_order, order.orig_order.volume, order.orig_order.price,
                               tick.exchange_timestamp_us);
          id_price_map_.erase(order.orig_order.order_id);
        }
        orderbook.erase(begin);
      }
    }

    for (int level = 0; level < kMaxMarketLevel; ++level) {
      if (tick.ask_volume[level] == 0) {
        continue;
      }
      auto order_list_it = orderbook.find(u64_price(tick.ask[level]));
      if (order_list_it != orderbook.end()) {
        auto& order_list = order_list_it->second;
        for (auto order_it = order_list.begin(); order_it != order_list.end();) {
          auto& order = *order_it;
          LOG_DEBUG("queue_position:{} ask_filled:{}", order.queue_position, ask_filled);
          if (order.queue_position < ask_filled) {
            listener()->OnTraded(order.orig_order, order.orig_order.volume, order.orig_order.price,
                                 tick.exchange_timestamp_us);
            id_price_map_.erase(order.orig_order.order_id);
            order_it = order_list.erase(order_it);
          } else {
            order.queue_position -= ask_filled;
            ++order_it;
          }
          if (order_list.empty()) {
            orderbook.erase(order_list_it);
          }
        }
      }
      ask_filled -= tick.ask_volume[level];
      if (ask_filled <= 0) {
        break;
      }
    }
  }

  // 空方小于bid价格的单应该全部成交
  if (tick.bid_volume[0] > 0) {
    auto& orderbook = ask_orderbooks_[tick.ticker_id - 1];
    while (!orderbook.empty()) {
      auto begin = orderbook.begin();
      auto price_u64 = begin->first;
      if (price_u64 > u64_price(tick.bid[0])) {
        break;
      }
      auto& order_list = begin->second;
      for (auto& order : order_list) {
        listener()->OnTraded(order.orig_order, order.orig_order.volume, order.orig_order.price,
                             tick.exchange_timestamp_us);
        id_price_map_.erase(order.orig_order.order_id);
      }
      orderbook.erase(begin);
    }
  }

  // 多方大于ask价格的单应该全部成交
  if (tick.ask_volume[0] > 0) {
    auto& orderbook = bid_orderbooks_[tick.ticker_id - 1];
    while (!orderbook.empty()) {
      auto begin = orderbook.begin();
      auto price_u64 = begin->first;
      if (price_u64 < u64_price(tick.ask[0])) {
        break;
      }
      auto& order_list = begin->second;
      for (auto& order : order_list) {
        listener()->OnTraded(order.orig_order, order.orig_order.volume, order.orig_order.price,
                             tick.exchange_timestamp_us);
        id_price_map_.erase(order.orig_order.order_id);
      }
      orderbook.erase(begin);
    }
  }

  ticks_[tick.ticker_id - 1] = tick;
}

}  // namespace ft
