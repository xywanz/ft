// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_INCLUDE_FT_UTILS_REDIS_POSITION_HELPER_H_
#define FT_INCLUDE_FT_UTILS_REDIS_POSITION_HELPER_H_

#include <fmt/format.h>
#include <ft/base/trade_msg.h>
#include <ft/utils/redis.h>

#include <string>

namespace ft {

class RedisPositionGetter {
 public:
  RedisPositionGetter() {}

  void SetAccount(uint64_t account) {
    account_abbreviation_ = std::to_string(account).substr(0, 4);
    pos_key_prefix_ = fmt::format("pos-{}-", account_abbreviation_);
  }

  bool get(const std::string& ticker, Position* pos) const {
    auto reply = redis_.get(fmt::format("{}{}", pos_key_prefix_, ticker));
    if (!reply || reply->len == 0) return false;

    *pos = *reinterpret_cast<const Position*>(reply->str);
    return true;
  }

 protected:
  RedisSession redis_;

  std::string account_abbreviation_;
  std::string pos_key_prefix_;
};

class RedisPositionSetter : public RedisPositionGetter {
 public:
  RedisPositionSetter() {}

  bool get(const std::string& ticker, Position* pos) const {
    auto reply = redis_.get(fmt::format("{}{}", pos_key_prefix_, ticker));
    if (!reply) return false;

    *pos = *reinterpret_cast<const Position*>(reply->str);
    return true;
  }

  void set(const std::string& ticker, const Position& pos) {
    redis_.set(fmt::format("{}{}", pos_key_prefix_, ticker), &pos, sizeof(pos));
  }

  void Clear() {
    auto reply = redis_.keys(fmt::format("{}*", pos_key_prefix_));
    for (size_t i = 0; i < reply->elements; ++i) redis_.del(reply->element[i]->str);
  }
};

}  // namespace ft

#endif  // FT_INCLUDE_FT_UTILS_REDIS_POSITION_HELPER_H_
