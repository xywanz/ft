// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_SRC_TRADING_SERVER_DATASTRUCT_PROTOCOL_H_
#define FT_SRC_TRADING_SERVER_DATASTRUCT_PROTOCOL_H_

#include <fmt/format.h>

#include <cstdint>
#include <string>

namespace ft {

using StrategyIdType = char[16];

/*
 * 这部分是Strategy和TradingEngine之间的交互协议
 * Strategy通过IPC向TradingEngine发送交易相关指令
 */

inline const uint32_t TRADER_CMD_MAGIC = 0x1709394;

enum TraderCmdType {
  CMD_NEW_ORDER = 1,
  CMD_CANCEL_ORDER,
  CMD_CANCEL_TICKER,
  CMD_CANCEL_ALL,
};

struct TraderOrderReq {
  uint32_t client_order_id;
  uint32_t ticker_id;
  uint32_t direction;
  uint32_t offset;
  uint32_t type;
  int volume;
  double price;

  uint32_t flags;
  bool without_check;
} __attribute__((packed));

struct TraderCancelReq {
  uint64_t order_id;
} __attribute__((packed));

struct TraderCancelTickerReq {
  uint32_t ticker_id;
} __attribute__((packed));

struct TraderCommand {
  uint32_t magic;
  uint32_t type;
  StrategyIdType strategy_id;
  union {
    TraderOrderReq order_req;
    TraderCancelReq cancel_req;
    TraderCancelTickerReq cancel_ticker_req;
  };
} __attribute__((packed));

/*
 *
 */
struct OrderResponse {
  uint32_t client_order_id;
  uint32_t order_id;
  uint32_t ticker_id;
  uint32_t direction;
  uint32_t offset;
  int original_volume;
  int traded_volume;

  bool completed;
  int error_code;
  uint32_t this_traded;
  double this_traded_price;
} __attribute__((packed));

}  // namespace ft

#endif  // FT_SRC_TRADING_SERVER_DATASTRUCT_PROTOCOL_H_
