// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "Indicator/MA.h"

namespace ft {

void MA::on_init(const Candlestick* candlestick) {
  candlestick_ = candlestick;

  // 计算当日历史行情
  for (std::size_t offset = candlestick_->get_bar_count(); offset > 0; --offset)
    update(candlestick_->get_bar(offset - 1));
}

void MA::on_bar() { update(candlestick_->get_bar()); }

void MA::update(const Bar* bar) {
  ++bar_count_;
  sum_ += bar->close;

  if (bar_count_ < period_) return;

  if (bar_count_ > period_) sum_ -= candlestick_->get_bar(period_)->close;
  ma_.emplace_back(sum_ / period_);
}

}  // namespace ft
