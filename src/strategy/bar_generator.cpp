// Copyright [2020-2021] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "ft/strategy/bar_generator.h"

namespace ft {

BarGenerator::BarGenerator() {}

void BarGenerator::AddPeriod(BarPeriod period, Callback&& cb) { m1_cb_ = std::move(cb); }

void BarGenerator::OnTick(const TickData& tick) {
  if (tick.last_price < 1e-6) {
    return;
  }

  uint64_t current_min = tick.exchange_timestamp_us / 60000000UL;
  bool new_1m_bar = false;
  BarData* bar_m1;

  auto bar_iter = bars_.find(tick.ticker_id);
  if (bar_iter == bars_.end()) {
    new_1m_bar = true;
    bar_m1 = &bars_[tick.ticker_id];
    bar_m1->ticker_id = tick.ticker_id;
  } else if (bar_iter->second.timestamp_us / 60000000UL != current_min) {
    bar_m1 = &bar_iter->second;
    if (m1_cb_) {
      m1_cb_(*bar_m1);
      new_1m_bar = true;
    }
  } else {
    bar_m1 = &bar_iter->second;
  }

  if (new_1m_bar) {
    bar_m1->timestamp_us = tick.exchange_timestamp_us;
    bar_m1->open = tick.last_price;
    bar_m1->high = tick.last_price;
    bar_m1->low = tick.last_price;
    bar_m1->close = tick.last_price;
  } else {
    bar_m1->high = std::max(bar_m1->high, tick.last_price);
    bar_m1->low = std::min(bar_m1->low, tick.last_price);
    bar_m1->close = tick.last_price;
  }
}

}  // namespace ft
