// Copyright [2020-2021] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_INCLUDE_FT_BASE_MARKET_DATA_H_
#define FT_INCLUDE_FT_BASE_MARKET_DATA_H_

#include <cstdint>

#include "ft/component/serializable.h"
#include "ft/utils/datetime.h"

namespace ft {

constexpr int kMaxMarketLevel = 10;

enum class MarketDataSource : uint8_t {
  kCTP = 1,
  kXTP = 2,
};

struct TickData : public pubsub::Serializable<TickData> {
  MarketDataSource source;
  uint64_t local_timestamp_us;
  datetime::Datetime exchange_datetime;

  uint32_t ticker_id;
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

  double ask[kMaxMarketLevel]{0};
  double bid[kMaxMarketLevel]{0};
  int ask_volume[kMaxMarketLevel]{0};
  int bid_volume[kMaxMarketLevel]{0};

  struct {
    double iopv;
  } etf;

  SERIALIZABLE_FIELDS(source, local_timestamp_us, exchange_datetime, ticker_id, last_price,
                      open_price, highest_price, lowest_price, pre_close_price, upper_limit_price,
                      lower_limit_price, volume, turnover, open_interest, ask, bid, ask_volume,
                      bid_volume, etf.iopv);
};

}  // namespace ft

#endif  // FT_INCLUDE_FT_BASE_MARKET_DATA_H_
