// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_INCLUDE_IPC_REDIS_MD_HELPER_H_
#define FT_INCLUDE_IPC_REDIS_MD_HELPER_H_

#include <fmt/format.h>

#include <string>
#include <vector>

#include "core/contract_table.h"
#include "core/tick_data.h"
#include "ipc/redis.h"

namespace ft {

class RedisMdPusher {
 public:
  RedisMdPusher() {}

  void push(const std::string& ticker, const TickData& tick) {
    redis_.publish(fmt::format("quote-{}", ticker), &tick, sizeof(tick));
  }

 private:
  RedisSession redis_;
};

class RedisTERspPuller {
 public:
  RedisTERspPuller() {}

  void subscribe_order_rsp(const std::string strategy_id) {
    redis_.subscribe({strategy_id});
  }

  void subscribe_md(const std::vector<std::string> ticker_vec) {
    for (const auto& ticker : ticker_vec)
      redis_.subscribe({fmt::format("quote-{}", ticker)});
  }

  RedisReply pull() { return redis_.get_sub_reply(); }

 private:
  RedisSession redis_;
};

}  // namespace ft

#endif  // FT_INCLUDE_IPC_REDIS_MD_HELPER_H_
