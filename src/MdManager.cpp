// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "MdManager.h"

#include <cassert>

namespace ft {

MdManager::MdManager(const std::string& ticker)
  : ticker_(ticker) {
}

void MdManager::on_tick(const MarketData* data) {
  // if (last_mininute_ == 0) {
  //   last_mininute_ = data->time_ms / 6000;
  //   last_kline_m1_.open = data->last_price;
  //   last_kline_m1_.close = data->last_price;
  //   last_kline_m1_.high = data->last_price;
  //   last_kline_m1_.low = data->last_price;
  // }

  // uint64_t current = data->time_ms / 6000;
  // if (current > last_mininute_) {
  //   kchart_m1_.emplace_back(last_kline_m1_);
  //   last_kline_m1_.open = data->last_price;
  //   last_kline_m1_.close = data->last_price;
  //   last_kline_m1_.high = data->last_price;
  //   last_kline_m1_.low = data->last_price;
  // } else if (current == last_mininute_) {
  //   last_kline_m1_.close = data->last_price;
  //   if (data->last_price > last_kline_m1_.high)
  //     last_kline_m1_.high = data->last_price;
  //   else if (data->last_price < last_kline_m1_.low)
  //     last_kline_m1_.low = data->last_price;
  // } else {
  //   assert(false);
  // }

  last_kline_m1_.open = data->last_price;
  last_kline_m1_.close = data->last_price;
  last_kline_m1_.high = data->last_price;
  last_kline_m1_.low = data->last_price;
  kchart_m1_.emplace_back(last_kline_m1_);

  volume_ = data->volume;
  open_interest_ = data->open_interest;
  turnover_ = data->turnover;
}

}  // namespace ft
