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

struct OrderAcceptedRsp {
  uint64_t order_id;
};

struct OrderRejectedRsp {
  uint64_t order_id;
  std::string reason;
};

struct OrderCanceledRsp {
  uint64_t order_id;
  int canceled_volume;
};

struct OrderCancelRejectedRsp {
  uint64_t order_id;
  std::string reason;
};

struct OrderTradedRsp {
  uint64_t timestamp_us;
  uint64_t order_id;
  double price;
  int volume;
};

enum class GatewayMsgType : uint32_t {
  kOrderAcceptedRsp = 1,
  kOrderOrderTradedRsp,
  kOrderRejectedRsp,
  kOrderCanceledRsp,
  kOrderCancelRejectedRsp,
  kAccount,
  kAccountEnd,
  kPosition,
  kPositionEnd,
  kOrder,
  kOrderEnd,
  kTrade,
  kTradeEnd,
  kContract,
  kContractEnd,
};

struct GatewayOrderResponse {
  GatewayMsgType msg_type;
  std::variant<OrderAcceptedRsp, OrderTradedRsp, OrderRejectedRsp, OrderCanceledRsp,
               OrderCancelRejectedRsp>
      data;
};

struct GatewayQueryResult {
  GatewayMsgType msg_type;
  std::variant<Account, Position, HistoricalOrder, HistoricalTrade, Contract> data;
};

}  // namespace ft

#endif  // FT_INCLUDE_FT_TRADER_MSG_H_
