// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "MarketData/TickDB.h"

#include <cassert>

namespace ft {

TickDB::TickDB(uint64_t ticker_index) : ticker_index_(ticker_index) {}

void TickDB::process_tick(const TickData* data) {
  tick_data_.emplace_back(*data);
}

}  // namespace ft
