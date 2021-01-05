// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef BSS_PROTOCOL_FAST_PROTO_H_
#define BSS_PROTOCOL_FAST_PROTO_H_

#include "protocol/protocol.h"

namespace ft::bss {

struct NewBoardLotMarketOrder {
  OrderId client_order_id;              // 0: Y
  BrokerId submitting_broker_id;        // 1: Y
  SecurityId security_id;               // 2: Y
  SecurityIdSource security_id_source;  // 3: Y
  SecurityExchange security_exchange;   // 4: N
  BrokerLocationId broker_location_id;  // 5: N
  TransactionTime transaction_time;     // 6: Y
  Side side;                            // 7: Y, 1 = buy, 2 = sell, 5 = sell short
  OrderType order_type;                 // 8: Y. 1 = market, 2 = limit
  // 9: N. if price is 1.23, set this field to 123,000,000 (1.23 * 1e8)
  Price price;
  // 10: Y. if quantity is 100, set this field to 10,000,000,000 (100 * 1e8)
  Quantity order_quantity;
  Tif tif;  // 11: N. 0 = day, 3 = IOC, 4 = FOK, 9 = at crossing
  // 12: N. 1 = Close
  // applicable only if side = 1(buy) to indicatecovering a short sell
  PositionEffect position_effect;
  OrderRestrictions order_restrictions;  // 13: N
  // 14: N. if present, this should be set as 1
  MaxPriceLevels max_price_levels;
  OrderCapacity order_capacity;                    // 15: N. 1 = Agent, 2 = principal
  Text text;                                       // 16: N
  ExecutionInstructions execution_instructions;    // 17: N
  DisclosureInstructions disclosure_instructions;  // 18: Y
  LotType lot_type;                                // 19: N. not included = board lot, 1 = odd lot
} __attribute__((__packed__));

template <bool WithMaxPriceLevel>
struct NewBoardLotLimitOrder;

template <>
struct NewBoardLotLimitOrder<true> {
} __attribute__((__packed__));

template <>
struct NewBoardLotLimitOrder<false> {
} __attribute__((__packed__));

}  // namespace ft::bss

#endif  // BSS_PROTOCOL_FAST_PROTO_H_
