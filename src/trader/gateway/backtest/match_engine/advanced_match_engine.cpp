// Copyright [2020-present] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "trader/gateway/backtest/match_engine/advanced_match_engine.h"

#include "ft/base/contract_table.h"

namespace ft {

void AdvancedMatchEngine::OnNewTick(const TickData& new_tick) {
  if (new_tick.ask_volume[0] > 0) {
    uint64_t ask_u64 = u64_price(new_tick.ask[0]);
    while (!bid_levels_.empty()) {
      auto level_it = bid_levels_.begin();
      if (level_it->first < ask_u64) {
        break;
      }
      auto& order_list = level_it->second;
      for (auto& order : order_list) {
        // OnTrade
        (void)order;
      }
      bid_levels_.erase(level_it);
    }
  }

  if (new_tick.bid_volume[0] > 0) {
    uint64_t bid_u64 = u64_price(new_tick.bid[0]);
    while (!ask_levels_.empty()) {
      auto level_it = ask_levels_.begin();
      if (level_it->first > bid_u64) {
        break;
      }
      auto& order_list = level_it->second;
      for (auto& order : order_list) {
        // OnTrade
        (void)order;
      }
      bid_levels_.erase(level_it);
    }
  }

  if (new_tick.ask_volume[0] == 0 || new_tick.bid_volume[0] == 0) {
    return;
  }

  uint64_t delta_turnover = new_tick.turnover - current_tick_.turnover;
  uint64_t delta_volume = new_tick.volume - current_tick_.volume;
  auto* contract = ContractTable::get_by_index(new_tick.ticker_id);
  double avg_price = static_cast<double>(delta_turnover) / (delta_volume * contract->size);
  int bid_trade_volume = 0;
  int ask_trade_volume = 0;
  if (avg_price >= current_tick_.ask[0]) {
    ask_trade_volume = delta_volume;
  } else if (avg_price <= current_tick_.bid[0]) {
    bid_trade_volume = delta_volume;
  } else {
    double spread = current_tick_.ask[0] - current_tick_.bid[0];
    bid_trade_volume = static_cast<int>((avg_price - current_tick_.bid[0]) / spread * delta_volume);
    ask_trade_volume = delta_volume - bid_trade_volume;
  }

  if (ask_trade_volume != 0) {
    for (int i = 0; i < 5; ++i) {
      if (current_tick_.ask_volume[i] == 0) {
        continue;
      }
      uint64_t u64p = u64_price(current_tick_.ask[i]);
      auto level_it = ask_levels_.find(u64p);
      if (level_it != ask_levels_.end()) {
        auto& order_list = level_it->second;
        for (auto order_it = order_list.begin(); order_it != order_list.end();) {
          auto& order = *order_it;
          if (order.order_pos <= ask_trade_volume) {
            // OnTrade
            order_it = order_list.erase(order_it);
          } else {
            ++order_it;
          }
        }
      }
      ask_trade_volume -= current_tick_.ask_volume[i];
      if (ask_trade_volume <= 0) {
        break;
      }
    }
  }

  if (bid_trade_volume != 0) {
    for (int i = 0; i < 5; ++i) {
      if (current_tick_.bid_volume[i] == 0 || current_tick_.bid[i] > avg_price) {
        continue;
      }
      uint64_t u64p = u64_price(current_tick_.bid[i]);
      auto level_it = bid_levels_.find(u64p);
      if (level_it != bid_levels_.end()) {
        auto& order_list = level_it->second;
        for (auto order_it = order_list.begin(); order_it != order_list.end();) {
          auto& order = *order_it;
          if (order.order_pos <= bid_trade_volume) {
            // OnTrade
            order_it = order_list.erase(order_it);
          } else {
            ++order_it;
          }
        }
      }
      bid_trade_volume -= current_tick_.bid_volume[i];
      if (bid_trade_volume <= 0) {
        break;
      }
    }
  }

  current_tick_ = new_tick;
}

bool AdvancedMatchEngine::InsertOrder(const OrderRequest& order) {
  if (order.direction == Direction::kBuy) {
    AddLongOrder(order);
  } else {
    AddShortOrder(order);
  }
  return true;
}

void AdvancedMatchEngine::AddLongOrder(const OrderRequest& order) {
  if (order.type == OrderType::kLimit) {
    int volume_left = order.volume;
    int level_pos = 0;
    while (volume_left > 0 && level_pos < 5) {
      if (current_tick_.ask_volume[level_pos] > 0 && current_tick_.ask[level_pos] <= order.price) {
        int traded = std::min(current_tick_.ask_volume[level_pos], volume_left);
        volume_left -= traded;
        // OnTrade
      }
      ++level_pos;
    }

    if (volume_left > 0) {
      uint64_t u64price = u64_price(order.price);
      InnerOrder inner_order{};
      inner_order.orig_order = order;
      inner_order.traded = order.volume - volume_left;
      inner_order.order_pos = 0;

      if (inner_order.traded == 0) {
        level_pos = 0;
        while (level_pos < 5) {
          if (current_tick_.bid_volume[level_pos] > 0 &&
              u64_price(current_tick_.bid[level_pos]) == u64price) {
            inner_order.order_pos = current_tick_.bid_volume[level_pos] + 1;
            break;
          }
          ++level_pos;
        }
      }

      bid_levels_[u64price].emplace_back(inner_order);
    }
  } else {
    int volume_left = order.volume;
    int level_pos = 0;
    bool to_trade = true;

    if (order.type == OrderType::kFok) {
      while (volume_left > 0 && level_pos < 5) {
        if (current_tick_.ask_volume[level_pos] > 0 &&
            current_tick_.ask[level_pos] <= order.price) {
          volume_left -= std::min(current_tick_.ask_volume[level_pos], volume_left);
        }
        ++level_pos;
      }

      if (volume_left != 0) {
        to_trade = false;
      }
    }

    if (to_trade) {
      volume_left = order.volume;
      level_pos = 0;
      while (volume_left > 0 && level_pos < 5) {
        if (current_tick_.ask_volume[level_pos] > 0 &&
            current_tick_.ask[level_pos] <= order.price) {
          int traded = std::min(current_tick_.ask_volume[level_pos], volume_left);
          volume_left -= traded;
          // OnTrade
        }
        ++level_pos;
      }
    }

    if (volume_left != 0) {
      // OnCancel
    }
  }
}

void AdvancedMatchEngine::AddShortOrder(const OrderRequest& order) {
  if (order.type == OrderType::kLimit) {
    int volume_left = order.volume;
    int level_pos = 0;
    while (volume_left > 0 && level_pos < 5) {
      if (current_tick_.bid_volume[level_pos] > 0 && current_tick_.bid[level_pos] >= order.price) {
        int traded = std::min(current_tick_.bid_volume[level_pos], volume_left);
        volume_left -= traded;
        // OnTrade
      }
      ++level_pos;
    }

    if (volume_left > 0) {
      uint64_t u64price = u64_price(order.price);
      InnerOrder inner_order{};
      inner_order.orig_order = order;
      inner_order.traded = order.volume - volume_left;
      inner_order.order_pos = 0;

      if (inner_order.traded == 0) {
        level_pos = 0;
        while (level_pos < 5) {
          if (current_tick_.ask_volume[level_pos] > 0 &&
              u64_price(current_tick_.ask[level_pos]) == u64price) {
            inner_order.order_pos = current_tick_.ask_volume[level_pos] + 1;
            break;
          }
          ++level_pos;
        }
      }

      ask_levels_[u64price].emplace_back(inner_order);
    }
  } else {
    int volume_left = order.volume;
    int level_pos = 0;
    bool to_trade = true;

    if (order.type == OrderType::kFok) {
      while (volume_left > 0 && level_pos < 5) {
        if (current_tick_.bid_volume[level_pos] > 0 &&
            current_tick_.bid[level_pos] >= order.price) {
          volume_left -= std::min(current_tick_.bid_volume[level_pos], volume_left);
        }
        ++level_pos;
      }

      if (volume_left != 0) {
        to_trade = false;
      }
    }

    if (to_trade) {
      volume_left = order.volume;
      level_pos = 0;
      while (volume_left > 0 && level_pos < 5) {
        if (current_tick_.bid_volume[level_pos] > 0 &&
            current_tick_.bid[level_pos] >= order.price) {
          int traded = std::min(current_tick_.bid_volume[level_pos], volume_left);
          volume_left -= traded;
          // OnTrade
        }
        ++level_pos;
      }
    }

    if (volume_left != 0) {
      // OnCancel
    }
  }
}

bool AdvancedMatchEngine::CancelOrder(uint64_t order_id, uint32_t ticker_id) { return false; }

}  // namespace ft
