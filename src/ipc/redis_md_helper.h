// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_INCLUDE_IPC_REDIS_MD_HELPER_H_
#define FT_INCLUDE_IPC_REDIS_MD_HELPER_H_

#include <fmt/format.h>

#include <string>
#include <vector>

#include "ipc/redis.h"
#include "trading_server/datastruct/contract_table.h"
#include "trading_server/datastruct/tick_data.h"

namespace ft {

class RedisMdPusher {
 public:
  RedisMdPusher() {}

  void Push(const std::string& ticker, const TickData& tick) {
    redis_.publish(fmt::format("quote-{}", ticker), &tick, sizeof(tick));
  }

 private:
  RedisSession redis_;
};

class RedisTERspPuller {
 public:
  RedisTERspPuller() {}

  void SubscribeOrderResponse(const std::string strategy_id) { redis_.Subscribe({strategy_id}); }

  void SubscribeMarketData(const std::vector<std::string> ticker_vec) {
    for (const auto& ticker : ticker_vec) redis_.Subscribe({fmt::format("quote-{}", ticker)});
  }

  RedisReply Pull() { return redis_.get_sub_reply(); }

 private:
  RedisSession redis_;
};

}  // namespace ft

#endif  // FT_INCLUDE_IPC_REDIS_MD_HELPER_H_
