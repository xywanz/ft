// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_INCLUDE_MARKETDATA_CANDLESTICK_H_
#define FT_INCLUDE_MARKETDATA_CANDLESTICK_H_

#include <string>
#include <vector>

#include "Base/DataStruct.h"

namespace ft {

struct Bar {
  double high;
  double low;
  double open;
  double close;
};

class Candlestick {
 public:
  Candlestick() {}

  void on_tick(const TickData* tick);

  const Bar* get_bar(std::size_t offset) const {
    if (offset >= bars_.size())
      return nullptr;
    return &*(bars_.rbegin() + offset);
  }

 private:
  std::vector<Bar> bars_;
  uint64_t period_sec_ = 60;
  uint64_t curp_start_sec_ = 0;
  uint64_t nextp_start_sec_ = 0;
  uint64_t td_start_sec_ = 360 * 95;
};

}  // namespace ft

#endif  // FT_INCLUDE_MARKETDATA_CANDLESTICK_H_
