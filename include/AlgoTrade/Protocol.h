// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_INCLUDE_ALGOTRADE_PROTOCOL_H_
#define FT_INCLUDE_ALGOTRADE_PROTOCOL_H_

#include <fmt/format.h>

#include <memory>
#include <string>
#include <vector>

#include "Base/DataStruct.h"

namespace ft {

enum TraderCmdType { NEW_ORDER = 1, CANCEL_ORDER };

struct OrderReq {
  uint64_t ticker_index;
  Direction direction;
  Offset offset;
  OrderType type;
  int64_t volume;
  double price;
};

struct CancelReq {
  uint64_t ticker_index;
  uint64_t order_id;
};

struct TraderCommand {
  uint32_t type;
  union {
    OrderReq order_req;
    CancelReq cancel_req;
  };
};

constexpr const char* const TRADER_CMD_TOPIC = "trader_cmd";

inline std::string get_md_topic(const std::string& ticker) {
  return fmt::format("md-{}", ticker);
}

}  // namespace ft

#endif  // FT_INCLUDE_ALGOTRADE_PROTOCOL_H_
