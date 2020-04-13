// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_INCLUDE_MARKETDATA_H_
#define FT_INCLUDE_MARKETDATA_H_

#include <string>

#include "Common.h"

namespace ft {

static const std::size_t kMarketLevel = 10;

struct MarketData {
  std::string symbol;
  std::string exchange;
  std::string ticker;

  uint64_t    time_ms;

  double      last_price = 0;

  int         level = 0;
  double      ask[kMarketLevel] {0};
  double      bid[kMarketLevel] {0};
  uint64_t    ask_volume[kMarketLevel] {0};
  uint64_t    bid_volume[kMarketLevel] {0};

  double      open_price = 0;
  double      highest_price = 0;
  double      lowest_price = 0;
  double      close_price = 0;

  double      upper_limit_price = 0;
  double      lower_limit_price = 0;

  uint64_t    volume;
  uint64_t    turnover;

  uint64_t    open_interest;
};

}  // namespace ft

#endif  // FT_INCLUDE_MARKETDATA_H_
