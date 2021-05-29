// Copyright [2020-2021] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "ft/strategy/algo_order/target_pos_engine.h"

#include <stdexcept>

#include "ft/base/contract_table.h"
#include "ft/base/error_code.h"
#include "ft/utils/misc.h"

namespace ft {

TargetPosEngine::TargetPosEngine(int ticker_id) : ticker_id_(ticker_id) {
  auto* contract = ContractTable::get_by_index(ticker_id);
  if (!contract) {
    throw std::runtime_error("contract not found");
  }
  ticker_ = contract->ticker;

  client_order_id_ = 100000000UL + ticker_id_ * 100000UL;
}

void TargetPosEngine::Init() {
  auto current_pos = GetPosition(ticker_);
  long_pos_ = current_pos.long_pos.holdings;
  short_pos_ = current_pos.short_pos.holdings;
  target_pos_ = long_pos_ - short_pos_;
}

void TargetPosEngine::SetPriceLimit(double price_limit) {
  auto contract = ContractTable::get_by_index(ticker_id_);
  assert(contract);

  double v = price_limit / contract->price_tick;
  int64_t price_tick_num = std::floor(v);
  price_limit_ = price_tick_num * contract->price_tick;
}

void TargetPosEngine::SetTargetPos(int volume) { target_pos_ = volume; }

void TargetPosEngine::OnTick(const TickData& tick) {
  if (tick.ask[0] > 0.0) {
    ask_ = tick.ask[0];
  }
  if (tick.bid[0] > 0.0) {
    bid_ = tick.bid[0];
  }

  for (auto& [client_order_id, order] : orders_) {
    if (order.direction == Direction::kBuy && order.order_id != 0 && order.price < ask_) {
      CancelOrder(order.order_id);
    } else if (order.direction == Direction::kSell && order.order_id != 0 && order.price > bid_) {
      CancelOrder(order.order_id);
    }
  }

  if (long_pos_ - short_pos_ == target_pos_) {
    return;
  }

  int gap = target_pos_ - (long_pos_ + long_pending_ - short_pos_ - short_pending_);
  if (gap > 0) {
    if (ask_ < 1e-6) {
      return;
    }

    // 先尝试撤sell_close及sell_open
    if (short_pending_ > 0) {
      for (auto& [client_order_id, order] : orders_) {
        if (order.direction == Direction::kSell && order.order_id != 0) {
          CancelOrder(order.order_id);
          gap -= order.volume;
          if (gap <= 0) {
            break;
          }
        }
      }
    }

    if (gap > 0) {
      int buy_close_vol = std::min(short_pos_ - short_pending_, gap);
      int buy_open_vol = std::max(gap - buy_close_vol, 0);
      long_pending_ += buy_close_vol + buy_open_vol;

      if (buy_close_vol > 0) {
        SendOrder(Direction::kBuy, Offset::kCloseToday, buy_close_vol, ask_ + price_limit_);
      }

      if (buy_open_vol > 0) {
        SendOrder(Direction::kBuy, Offset::kOpen, buy_open_vol, ask_ + price_limit_);
      }
    }
  } else if (gap < 0) {
    if (bid_ < 1e-6) {
      return;
    }

    // 先尝试撤buy_close及buy_open
    if (long_pending_ > 0) {
      for (auto& [client_order_id, order] : orders_) {
        if (order.direction == Direction::kBuy && order.order_id != 0) {
          CancelOrder(order.order_id);
          gap += order.volume;
          if (gap >= 0) {
            break;
          }
        }
      }
    }

    if (gap < 0) {
      gap = -gap;
      int sell_close_vol = std::min(long_pos_ - long_pending_, gap);
      int sell_open_vol = std::max(gap - sell_close_vol, 0);
      short_pending_ += sell_close_vol + sell_open_vol;

      if (sell_close_vol > 0) {
        SendOrder(Direction::kSell, Offset::kCloseToday, sell_close_vol, bid_ - price_limit_);
      }

      if (sell_open_vol > 0) {
        SendOrder(Direction::kSell, Offset::kOpen, sell_open_vol, bid_ - price_limit_);
      }
    }
  }
}

void TargetPosEngine::OnOrder(const OrderResponse& order) {
  if (order.ticker_id != ticker_id_) {
    return;
  }

  auto it = orders_.find(order.client_order_id);
  if (it == orders_.end()) {
    return;
  }

  auto& pending_order = it->second;
  if (order.completed) {
    if (order.error_code != ErrorCode::kNoError && order.error_code <= ErrorCode::kSendFailed &&
        pending_order.order_id == 0) {
      if (pending_order.direction == Direction::kBuy) {
        long_pending_ -= pending_order.volume;
      } else {
        short_pending_ -= pending_order.volume;
      }
      orders_.erase(it);
      return;
    } else if (order.error_code == ErrorCode::kNoError &&
               pending_order.order_id == order.order_id) {
      if (pending_order.direction == Direction::kBuy) {
        long_pending_ -= (order.original_volume - order.traded_volume);
      } else {
        short_pending_ -= (order.original_volume - order.traded_volume);
      }
      orders_.erase(it);
      return;
    }
  } else {
    pending_order.order_id = order.order_id;
    return;
  }
}

void TargetPosEngine::OnTrade(const OrderResponse& trade) {
  if (trade.ticker_id != ticker_id_) {
    return;
  }

  if (trade.direction == Direction::kBuy) {
    if (IsOffsetOpen(trade.offset)) {
      long_pos_ += trade.this_traded;
    } else {
      short_pos_ -= trade.this_traded;
    }
    long_pending_ -= trade.this_traded;
  } else if (trade.direction == Direction::kSell) {
    if (IsOffsetOpen(trade.offset)) {
      short_pos_ += trade.this_traded;
    } else {
      long_pos_ -= trade.this_traded;
    }
    short_pending_ -= trade.this_traded;
  }
}

}  // namespace ft
