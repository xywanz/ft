// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_SRC_GATEWAY_XTP_XTP_COMMON_H_
#define FT_SRC_GATEWAY_XTP_XTP_COMMON_H_

#include <ctime>
#include <memory>
#include <string>

#include "ft/base/trade_msg.h"
#include "xtp_trader_api.h"

namespace ft {

struct XtpApiDeleter {
  template <class T>
  void operator()(T* p) {
    if (p) {
      p->RegisterSpi(nullptr);
      p->Release();
    }
  }
};

class XtpDatetimeConverter {
 public:
  void UpdateDate(int64_t dt) {
    int64_t new_date = dt / 1000000000L;
    if (date_ != new_date) {
      date_ = new_date;
      auto date_str = std::to_string(date_);
      struct tm tmp_tm;
      strptime(date_str.c_str(), "%Y%m%d", &tmp_tm);
      time_t t = mktime(&tmp_tm);
      today_timestamp_us_ = t * 1000000UL;
    }
  }

  uint64_t GetExchTimeStamp(int64_t dt) {
    uint64_t ms = dt % 1000;
    uint64_t sec = static_cast<uint64_t>((dt / 1000L) % 100L);
    uint64_t min = static_cast<uint64_t>((dt / 100000L) % 100L);
    uint64_t hour = static_cast<uint64_t>((dt / 10000000L) % 100L);
    return today_timestamp_us_ + hour * 3600000000UL + min * 60000000UL + sec * 1000000UL +
           ms * 1000UL;
  }

 private:
  int64_t date_ = 0;
  uint64_t today_timestamp_us_ = 0;
};

template <class T>
using XtpUniquePtr = std::unique_ptr<T, XtpApiDeleter>;

inline bool is_error_rsp(XTPRI* error_info) { return error_info && error_info->error_id != 0; }

inline std::string ft_exchange_type(XTP_EXCHANGE_TYPE exchange) {
  if (exchange == XTP_EXCHANGE_SH)
    return exchange::kSSE;
  else if (exchange == XTP_EXCHANGE_SZ)
    return exchange::kSZE;
  else
    return "UNKNOWN";
}

inline XTP_MARKET_TYPE xtp_market_type(const std::string& type) {
  if (type == exchange::kSSE)
    return XTP_MKT_SH_A;
  else if (type == exchange::kSZE)
    return XTP_MKT_SZ_A;
  else
    return XTP_MKT_UNKNOWN;
}

inline uint8_t xtp_side(Direction direction) {
  if (direction == Direction::kBuy)
    return XTP_SIDE_BUY;
  else if (direction == Direction::kSell)
    return XTP_SIDE_SELL;
  return XTP_SIDE_UNKNOWN;
}

inline XTP_PRICE_TYPE xtp_price_type(OrderType order_type) {
  if (order_type == OrderType::kLimit)
    return XTP_PRICE_LIMIT;
  else if (order_type == OrderType::kMarket)
    return XTP_PRICE_BEST5_OR_CANCEL;
  else if (order_type == OrderType::kBest)
    return XTP_PRICE_REVERSE_BEST_LIMIT;
  else if (order_type == OrderType::kFak)
    return XTP_PRICE_BEST5_OR_CANCEL;
  else if (order_type == OrderType::kFok)
    return XTP_PRICE_ALL_OR_CANCEL;
  else
    return XTP_PRICE_TYPE_UNKNOWN;
}

}  // namespace ft

#endif  // FT_SRC_GATEWAY_XTP_XTP_COMMON_H_
