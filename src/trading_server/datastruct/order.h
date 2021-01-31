// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_SRC_TRADING_SERVER_DATASTRUCT_ORDER_H_
#define FT_SRC_TRADING_SERVER_DATASTRUCT_ORDER_H_

#include <map>
#include <string>

#include "trading_server/datastruct/constants.h"
#include "trading_server/datastruct/contract.h"

namespace ft {

// OMS发给Gateway的订单请求
struct OrderRequest {
  uint64_t order_id;
  const Contract* contract;
  OrderType type;
  Direction direction;
  Offset offset;
  int volume;
  double price;
  OrderFlag flags;
} __attribute__((packed));

enum class OrderStatus {
  CREATED = 0,
  SUBMITTING,
  REJECTED,
  NO_TRADED,
  PART_TRADED,
  ALL_TRADED,
  CANCELED,
  CANCEL_REJECTED
};

struct Order {
  OrderRequest req;

  // 这个ID是策略发单的时候提供的，使策略能定位其订单，类似于备注
  uint32_t client_order_id;

  bool accepted = false;
  int traded_volume = 0;
  int canceled_volume = 0;
  OrderStatus status;
  uint64_t privdata;
  uint64_t insert_time;
  std::string strategy_id;
};

}  // namespace ft

#endif  // FT_SRC_TRADING_SERVER_DATASTRUCT_ORDER_H_
