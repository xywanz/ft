// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "ft/component/order_book/order_book.h"

namespace ft::orderbook {

void OrderBook::AddOrder(LimitOrder* order) {
  auto* level = FindOrCreateLevel(order->price(), order->decimal_price(), order->is_buy());
  order->set_level(level);
  level->AddOrder(order);
  order_map_.emplace(order->id(), *order);
}

void OrderBook::ModifyOrder(LimitOrder* order) {
  auto it = order_map_.find(order->id());
  if (it == order_map_.end()) {
    auto* level = FndLevel(order->decimal_price(), order->is_buy());
    level->AddOrder(order);
    order_map_.emplace(order->id(), *order);
  } else {
    auto& ori_order = it->second;
    auto* ori_level = FndLevel(ori_order.decimal_price(), ori_order.is_buy());
    assert(ori_level);
    if (ori_order.decimal_price() != order->decimal_price()) {
      ori_level->RemoveOrder(&ori_order);
      if (ori_level->total_volume() == 0) {
        if (ori_order.is_buy())
          buy_levels_.erase(ori_level->decimal_price());
        else
          sell_levels_.erase(ori_level->decimal_price());
      }
      auto new_level = FindOrCreateLevel(order->price(), order->decimal_price(), order->is_buy());
      new_level->AddOrder(order);
      order->set_level(new_level);
    } else {
      ori_level->ModifyOrder(order);
      order->set_level(ori_level);
    }
    ori_order = *order;
  }
}

void OrderBook::RemoveOrder(LimitOrder* order) {
  auto it = order_map_.find(order->id());
  if (it == order_map_.end()) return;
  order_map_.erase(it);
  auto* level = FndLevel(order->decimal_price(), order->is_buy());
  assert(level);
  level->RemoveOrder(order);
  if (level->total_volume() == 0) {
    if (order->is_buy())
      buy_levels_.erase(order->decimal_price());
    else
      sell_levels_.erase(order->decimal_price());
  }
}

void OrderBook::to_tick(TickData* tick) {
  tick->ticker_id = tid_;

  int i = 0;
  for (auto& [_, bid_level] : buy_levels_) {
    tick->bid[i] = bid_level.price();
    tick->bid_volume[i] = bid_level.total_volume();
    ++i;
    if (i == kMaxMarketLevel) break;
  }
  i = 0;
  for (auto& [_, ask_level] : sell_levels_) {
    tick->ask[i] = ask_level.price();
    tick->ask_volume[i] = ask_level.total_volume();
    ++i;
    if (i == kMaxMarketLevel) break;
  }
}

}  // namespace ft::orderbook
