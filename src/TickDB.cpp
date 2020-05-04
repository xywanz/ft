// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "MarketData/TickDB.h"

#include <cassert>

namespace ft {

TickDB::TickDB(const std::string& ticker) : ticker_(ticker) {}

void TickDB::process_tick(const TickData* data) {
  tick_data_.emplace_back(*data);
}

}  // namespace ft
