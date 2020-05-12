// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_INCLUDE_ALGOTRADE_TRADERPROTOCOL_H_
#define FT_INCLUDE_ALGOTRADE_TRADERPROTOCOL_H_

#include <fmt/format.h>

#include <memory>
#include <string>
#include <vector>

namespace ft {

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

#endif  // FT_INCLUDE_ALGOTRADE_TRADERPROTOCOL_H_
