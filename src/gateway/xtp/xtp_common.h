// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_SRC_GATEWAY_XTP_XTP_COMMON_H_
#define FT_SRC_GATEWAY_XTP_XTP_COMMON_H_

#include <xtp_trader_api.h>

#include <string>

#include "core/constants.h"

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

inline bool is_error_rsp(XTPRI* error_info) {
  return error_info && error_info->error_id != 0;
}

inline std::string ft_exchange_type(XTP_EXCHANGE_TYPE exchange) {
  if (exchange == XTP_EXCHANGE_TYPE::XTP_EXCHANGE_SH)
    return SSE;
  else if (exchange == XTP_EXCHANGE_TYPE::XTP_EXCHANGE_SZ)
    return SZE;
  else
    return "UNKNOWN";
}

inline XTP_MARKET_TYPE xtp_market_type(const std::string& type) {
  if (type == SSE)
    return XTP_MARKET_TYPE::XTP_MKT_SH_A;
  else if (type == SZE)
    return XTP_MARKET_TYPE::XTP_MKT_SZ_A;
  else
    return XTP_MARKET_TYPE::XTP_MKT_UNKNOWN;
}

inline uint8_t xtp_side(uint32_t direction) {
  if (direction == Direction::BUY)
    return XTP_SIDE_BUY;
  else if (direction == Direction::SELL)
    return XTP_SIDE_SELL;
  return XTP_SIDE_UNKNOWN;
}

inline XTP_PRICE_TYPE xtp_price_type(uint32_t order_type) {
  if (order_type == OrderType::LIMIT)
    return XTP_PRICE_TYPE::XTP_PRICE_LIMIT;
  else if (order_type == OrderType::MARKET)
    return XTP_PRICE_TYPE::XTP_PRICE_REVERSE_BEST_LIMIT;
  else if (order_type == OrderType::BEST)
    return XTP_PRICE_TYPE::XTP_PRICE_REVERSE_BEST_LIMIT;
  else if (order_type == OrderType::FAK)
    return XTP_PRICE_TYPE::XTP_PRICE_BEST_OR_CANCEL;
  else if (order_type == OrderType::FOK)
    return XTP_PRICE_TYPE::XTP_PRICE_LIMIT_OR_CANCEL;
  else
    return XTP_PRICE_TYPE::XTP_PRICE_TYPE_UNKNOWN;
}

}  // namespace ft

#endif  // FT_SRC_GATEWAY_XTP_XTP_COMMON_H_
