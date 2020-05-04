// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "DataCenter.h"

#include <map>
#include <string>
#include <vector>

namespace ft {

const Candlestick* DataCenter::load_candlestick(const std::string& ticker) {
  auto db_iter = tick_center_.find(ticker);
  if (db_iter == tick_center_.end()) return nullptr;

  {
    auto iter = candlestick_center_.find(ticker);
    if (iter != candlestick_center_.end()) return &iter->second;

    auto res = candlestick_center_.emplace(ticker, Candlestick());
    res.first->second.on_init(&db_iter->second);
    return &res.first->second;
  }
}

void DataCenter::process_tick(const TickData* tick) {
  {
    auto iter = tick_center_.find(tick->ticker);
    if (iter == tick_center_.end()) {
      auto res = tick_center_.emplace(tick->ticker, TickDB(tick->ticker));
      res.first->second.process_tick(tick);
    } else {
      iter->second.process_tick(tick);
    }
  }

  {
    auto iter = candlestick_center_.find(tick->ticker);
    if (iter != candlestick_center_.end()) iter->second.on_tick();
  }
}

}  // namespace ft
