// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_SRC_GATEWAY_XTP_XTP_COMMON_H_
#define FT_SRC_GATEWAY_XTP_XTP_COMMON_H_

#include <ft/base/trade_msg.h>
#include <xtp_trader_api.h>

#include <memory>
#include <string>

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
  else if (direction == Direction::kPurchase)
    return XTP_SIDE_PURCHASE;
  else if (direction == Direction::kRedeem)
    return XTP_SIDE_REDEMPTION;
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

// 仅适用于ETF申赎
inline TradeType ft_trade_type(XTP_SIDE_TYPE side, TXTPTradeTypeType type) {
  if (side == XTP_SIDE_PURCHASE) {
    if (type == XTP_TRDT_COMMON)
      return TradeType::kReleaseStock;
    else if (type == XTP_TRDT_CASH || type == XTP_TRDT_CROSS_MKT_CASH)
      return TradeType::kCashSubstitution;
    else if (type == XTP_TRDT_PRIMARY)
      return TradeType::kPrimaryMarket;
  } else if (side == XTP_SIDE_REDEMPTION) {
    if (type == XTP_TRDT_COMMON)
      return TradeType::kAcquireStock;
    else if (type == XTP_TRDT_CASH || type == XTP_TRDT_CROSS_MKT_CASH)
      return TradeType::kCashSubstitution;
    else if (type == XTP_TRDT_PRIMARY)
      return TradeType::kPrimaryMarket;
  }

  return TradeType::kUnknown;
}

}  // namespace ft

#endif  // FT_SRC_GATEWAY_XTP_XTP_COMMON_H_
