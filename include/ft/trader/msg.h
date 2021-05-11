// Copyright [2020-2021] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_INCLUDE_FT_TRADER_MSG_H_
#define FT_INCLUDE_FT_TRADER_MSG_H_

#include <string>
#include <variant>

#include "ft/base/trade_msg.h"

namespace ft {

struct OrderRequest {
  const Contract* contract;
  uint64_t order_id;
  OrderType type;
  Direction direction;
  Offset offset;
  int volume;
  double price;
  OrderFlag flags;
};

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
  Direction direction;
  Offset offset;
  TradeType trade_type;
  int volume;
  double price;
  double amount;

  datetime::Datetime trade_time;
};

enum class GatewayMsgType : uint32_t {
  kOrderAcceptance = 1,
  kOrderTrade,
  kOrderRejection,
  kOrderCancellation,
  kOrderCancelRejection,
  kAccount,
  kAccountEnd,
  kPosition,
  kPositionEnd,
  kTrade,
  kTradeEnd,
  kContract,
  kContractEnd,
};

struct GatewayOrderResponse {
  GatewayMsgType msg_type;
  std::variant<OrderAcceptance, Trade, OrderRejection, OrderCancellation, OrderCancelRejection>
      data;
};

struct GatewayQueryResult {
  GatewayMsgType msg_type;
  std::variant<Account, Position, Trade, Contract> data;
};

}  // namespace ft

#endif  // FT_INCLUDE_FT_TRADER_MSG_H_
