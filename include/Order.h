// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_INCLUDE_ORDER_H_
#define FT_INCLUDE_ORDER_H_

#include <string>

#include "Common.h"

namespace ft {

const uint32_t kCancelBit = 0;

struct Order {
  Order() {
  }

  Order(const std::string& _ticker,
        Direction _direction,
        Offset _offset,
        int _volume,
        OrderType _type,
        double _price)
    : ticker(_ticker),
      direction(_direction),
      offset(_offset),
      volume(_volume),
      type(_type),
      price(_price) {
    ticker_split(_ticker, &symbol, &exchange);
  }

  // req data
  std::string     symbol;
  std::string     exchange;
  std::string     ticker;
  std::string     order_id;
  OrderType       type;
  Direction       direction;
  Offset          offset;
  double          price = 0;
  int             volume = 0;

  // rsp or local data
  OrderStatus     status;
  int             volume_traded = 0;
  std::string     insert_time;
};

}  // namespace ft

#endif  // FT_INCLUDE_ORDER_H_
