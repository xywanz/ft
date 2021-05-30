// Copyright [2020-2021] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_INCLUDE_FT_BASE_MARKET_DATA_H_
#define FT_INCLUDE_FT_BASE_MARKET_DATA_H_

#include <cstdint>

namespace ft {

constexpr int kMaxMarketLevel = 5;

enum class MarketDataSource : uint8_t {
  kCTP = 1,
  kXTP = 2,
};

struct TickData {
  MarketDataSource source;
  uint64_t local_timestamp_us;
  uint64_t exchange_timestamp_us;

  uint32_t ticker_id;
  double last_price;
  double open_price;
  double highest_price;
  double lowest_price;
  double pre_close_price;
  double upper_limit_price;
  double lower_limit_price;
  uint64_t volume;
  uint64_t turnover;
  uint64_t open_interest;

  double ask[kMaxMarketLevel];
  double bid[kMaxMarketLevel];
  int ask_volume[kMaxMarketLevel];
  int bid_volume[kMaxMarketLevel];
} __attribute__((__aligned__(8)));

}  // namespace ft

#endif  // FT_INCLUDE_FT_BASE_MARKET_DATA_H_
