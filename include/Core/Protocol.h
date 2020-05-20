// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_INCLUDE_CORE_PROTOCOL_H_
#define FT_INCLUDE_CORE_PROTOCOL_H_

#include <fmt/format.h>

#include <cstdint>
#include <string>

namespace ft {

/*
 * 这部分是TradingEngine和Gateway之间的交互协议
 */

// 这个是TradingEngine发给Gateway的下单信息
struct OrderReq {
  uint64_t order_id;
  uint64_t ticker_index;
  uint64_t type;
  uint64_t direction;
  uint64_t offset;
  int64_t volume = 0;
  double price = 0;
} __attribute__((packed));

/*
 * 这部分是Strategy和TradingEngine之间的交互协议
 * Strategy通过IPC向TradingEngine发送交易相关指令
 */

inline const uint32_t TRADER_CMD_MAGIC = 0x1709394;

enum TraderCmdType { NEW_ORDER = 1, CANCEL_ORDER, CANCEL_TICKER, CANCEL_ALL };

struct TraderOrderReq {
  uint64_t ticker_index;
  uint64_t direction;
  uint64_t offset;
  uint64_t type;
  int64_t volume;
  double price;
} __attribute__((packed));

struct TraderCancelReq {
  uint64_t order_id;
} __attribute__((packed));

struct TraderCancelTickerReq {
  uint64_t ticker_index;
} __attribute__((packed));

struct TraderCommand {
  uint32_t magic;
  uint32_t type;
  uint32_t strategy_id;
  uint32_t reserved;
  union {
    TraderOrderReq order_req;
    TraderCancelReq cancel_req;
    TraderCancelTickerReq cancel_ticker_req;
  };
} __attribute__((packed));

constexpr const char* const TRADER_CMD_TOPIC = "trader_cmd";

inline std::string proto_md_topic(const std::string& ticker) {
  return fmt::format("md-{}", ticker);
}

inline std::string proto_pos_key(const std::string& ticker) {
  return fmt::format("pos-{}", ticker);
}

}  // namespace ft

#endif  // FT_INCLUDE_CORE_PROTOCOL_H_
