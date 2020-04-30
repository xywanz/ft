// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "MarketData/TickDatabase.h"

#include <cassert>

namespace ft {

TickDatabase::TickDatabase(const std::string& ticker)
  : ticker_(ticker) {
}

void TickDatabase::on_tick(const TickData* data) {
  tick_data_.emplace_back(*data);
}

}  // namespace ft
