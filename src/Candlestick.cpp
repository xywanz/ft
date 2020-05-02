// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "MarketData/Candlestick.h"

namespace ft {

void Candlestick::on_init(const TickDatabase* db) {
  db_ = db;

  for (std::size_t offset = db_->get_tick_count(); offset > 0; --offset)
    update(db_->get_tick(offset - 1));
}

// 没有考虑乱序问题，只要下一个周期的数据到了，那么宣告这个周期已经结束
void Candlestick::on_tick() {
  update(db_->get_tick());
}

void Candlestick::update(const TickData* tick) {
  if (tick->time_sec < td_start_sec_) {
    return;
  }

  if (nextp_start_sec_ == 0 || tick->time_sec >= nextp_start_sec_) {
    if (nextp_start_sec_ != 0 && tick->time_sec - nextp_start_sec_ >= 2 * period_sec_) {
      // 如果长时间没成交，那么没有行情推送，需要对之前的进行进行更新
      uint64_t gap = (tick->time_sec - nextp_start_sec_) / period_sec_ - 1;
      for (uint64_t i = 0; i < gap; ++i) {
        double price = bars_.back().close;
        bars_.push_back({price, price, price, price});
      }
    }

    auto time_diff = tick->time_sec - td_start_sec_;
    uint64_t cur_offset = time_diff / period_sec_;
    curp_start_sec_ = td_start_sec_ + cur_offset * period_sec_;
    nextp_start_sec_ = curp_start_sec_ + period_sec_;

    double price = tick->last_price;
    bars_.push_back({price, price, price, price});
  } else if (tick->time_sec >= curp_start_sec_) {
    auto& bar = bars_.back();
    if (tick->last_price > bar.high)
      bar.high = tick->last_price;
    else if (tick->last_price < bar.low)
      bar.low = tick->last_price;
    bar.close = tick->last_price;
  } else {
    spdlog::warn("[Candlestick::on_tick] Unexpected tick data.");
  }
}

}  // namespace ft
