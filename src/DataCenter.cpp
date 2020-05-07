// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "DataCenter.h"

#include <map>
#include <string>
#include <vector>

#include "ContractTable.h"

namespace ft {

const Candlestick* DataCenter::load_candlestick(const std::string& ticker) {
  const auto* contract = ContractTable::get_by_ticker(ticker);
  assert(contract);

  auto db_iter = tick_center_.find(contract->index);
  if (db_iter == tick_center_.end()) return nullptr;

  {
    auto iter = candlestick_center_.find(contract->index);
    if (iter != candlestick_center_.end()) return &iter->second;

    auto res = candlestick_center_.emplace(contract->index, Candlestick());
    res.first->second.on_init(&db_iter->second);
    return &res.first->second;
  }
}

void DataCenter::process_tick(const TickData* tick) {
  {
    auto iter = tick_center_.find(tick->ticker_index);
    if (iter == tick_center_.end()) {
      auto res =
          tick_center_.emplace(tick->ticker_index, TickDB(tick->ticker_index));
      res.first->second.process_tick(tick);
    } else {
      iter->second.process_tick(tick);
    }
  }

  {
    auto iter = candlestick_center_.find(tick->ticker_index);
    if (iter != candlestick_center_.end()) iter->second.on_tick();
  }
}

}  // namespace ft
