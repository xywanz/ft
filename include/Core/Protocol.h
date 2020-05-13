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
  uint64_t ticker_index;
  uint64_t type;
  uint64_t direction;
  uint64_t offset;
  int64_t volume = 0;
  double price = 0;
};

/*
 * 这部分是Strategy和TradingEngine之间的交互协议
 * Strategy通过IPC向TradingEngine发送交易相关指令
 */

enum TraderCmdType { NEW_ORDER = 1, CANCEL_ORDER };

struct TraderOrderReq {
  uint64_t ticker_index;
  uint64_t direction;
  uint64_t offset;
  uint64_t type;
  int64_t volume;
  double price;
};

struct TraderCancelReq {
  uint64_t ticker_index;
  uint64_t order_id;
};

struct TraderCommand {
  uint32_t type;
  union {
    TraderOrderReq order_req;
    TraderCancelReq cancel_req;
  };
};

constexpr const char* const TRADER_CMD_TOPIC = "trader_cmd";

inline std::string get_md_topic(const std::string& ticker) {
  return fmt::format("md-{}", ticker);
}

}  // namespace ft

#endif  // FT_INCLUDE_CORE_PROTOCOL_H_
