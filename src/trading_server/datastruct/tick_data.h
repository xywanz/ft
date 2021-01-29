// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_SRC_TRADING_SERVER_DATASTRUCT_TICK_DATA_H_
#define FT_SRC_TRADING_SERVER_DATASTRUCT_TICK_DATA_H_

#include <cstdint>
#include <string>

namespace ft {

inline const std::size_t kMaxMarketLevel = 10;
inline const std::size_t kDateLen = 9;

struct TickData {
  uint32_t source;
  uint32_t ticker_id;
  uint64_t date;
  uint64_t time_sec;
  uint64_t time_ms;

  double last_price = 0;
  double open_price = 0;
  double highest_price = 0;
  double lowest_price = 0;
  double pre_close_price = 0;
  double upper_limit_price = 0;
  double lower_limit_price = 0;
  uint64_t volume = 0;
  uint64_t turnover = 0;
  uint64_t open_interest = 0;

  int level = 0;
  double ask[kMaxMarketLevel]{0};
  double bid[kMaxMarketLevel]{0};
  int ask_volume[kMaxMarketLevel]{0};
  int bid_volume[kMaxMarketLevel]{0};

  struct {
    double iopv;
  } etf;
};

}  // namespace ft

#endif  // FT_SRC_TRADING_SERVER_DATASTRUCT_TICK_DATA_H_
