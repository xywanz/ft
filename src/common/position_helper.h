// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_SRC_COMMON_POSITION_HELPER_H_
#define FT_SRC_COMMON_POSITION_HELPER_H_

#include <cstring>
#include <string>
#include <vector>

#include "core/position.h"
#include "core/protocol.h"
#include "ipc/redis.h"

namespace ft {

class PositionHelper {
 public:
  void set_account(uint64_t account) { proto_.set_account(account); }

  Position get_position(const std::string& ticker) const {
    Position pos;

    auto reply = redis_.get(proto_.pos_key(ticker));
    if (reply->len == 0) return pos;

    memcpy(&pos, reply->str, sizeof(pos));
    return pos;
  }

  void get_all_positions(std::vector<Position>* pos_vec) const {
    auto reply = redis_.keys(proto_.pos_key_prefix());
    Position pos;

    for (size_t i = 0; i < reply->elements; ++i) {
      auto pos_reply = redis_.get(reply->element[i]->str);
      memcpy(&pos, pos_reply->str, sizeof(pos));
      pos_vec->emplace_back(pos);
    }
  }

  double get_realized_pnl() const {
    auto reply = redis_.get(proto_.rpnl_key());
    if (reply->len == 0) return 0;
    return *reinterpret_cast<double*>(reply->str);
  }

  double get_float_pnl() const {
    auto reply = redis_.get(proto_.fpnl_key());
    if (reply->len == 0) return 0;
    return *reinterpret_cast<double*>(reply->str);
  }

 private:
  RedisSession redis_;
  ProtocolQueryCenter proto_;
};

};  // namespace ft

#endif  // FT_SRC_COMMON_POSITION_HELPER_H_
