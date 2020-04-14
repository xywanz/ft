// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "MdManager.h"

#include <cassert>

namespace ft {

MdManager::MdManager(const std::string& ticker)
  : ticker_(ticker) {
}

void MdManager::on_tick(const MarketData* data) {
  tick_data_.emplace_back(*data);
}

}  // namespace ft
