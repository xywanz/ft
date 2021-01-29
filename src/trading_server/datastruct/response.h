// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_SRC_TRADING_SERVER_DATASTRUCT_RESPONSE_H_
#define FT_SRC_TRADING_SERVER_DATASTRUCT_RESPONSE_H_

#include <string>

/*
 * GATEWAY --> OrderManagementSystem
 */

namespace ft {

struct OrderAcceptance {
  uint64_t order_id;
};

struct OrderRejection {
  uint64_t order_id;
  std::string reason;
};

struct OrderCancellation {
  uint64_t order_id;
  int canceled_volume;
};

struct OrderCancelRejection {
  uint64_t order_id;
  std::string reason;
};

struct Trade {
  uint64_t order_id;
  uint32_t ticker_id;
  uint32_t direction;
  uint32_t offset;
  uint32_t trade_type;
  int volume;
  double price;
  double amount;
};

}  // namespace ft

#endif  // FT_SRC_TRADING_SERVER_DATASTRUCT_RESPONSE_H_
