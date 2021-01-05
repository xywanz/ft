// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_INCLUDE_IPC_REDIS_TRADER_CMD_HELPER_H_
#define FT_INCLUDE_IPC_REDIS_TRADER_CMD_HELPER_H_

#include <fmt/format.h>

#include <string>

#include "ipc/redis.h"
#include "trading_server/datastruct/contract_table.h"
#include "trading_server/datastruct/protocol.h"

namespace ft {

class RedisTraderCmdPusher {
 public:
  RedisTraderCmdPusher() {}

  void set_account(uint64_t account) {
    account_abbreviation_ = std::to_string(account).substr(0, 4);
    trader_cmd_topic_ = fmt::format("trader_cmd-{}", account_abbreviation_);
  }

  void Push(const TraderCommand& cmd) { redis_.publish(trader_cmd_topic_, &cmd, sizeof(cmd)); }

  std::string get_topic() const { return trader_cmd_topic_; }

 private:
  RedisSession redis_;

  std::string account_abbreviation_;
  std::string trader_cmd_topic_;
};

class RedisTraderCmdPuller {
 public:
  RedisTraderCmdPuller() {}

  void set_account(uint64_t account) {
    account_abbreviation_ = std::to_string(account).substr(0, 4);
    trader_cmd_topic_ = fmt::format("trader_cmd-{}", account_abbreviation_);
    redis_.Subscribe({trader_cmd_topic_});
  }

  RedisReply Pull() { return redis_.get_sub_reply(); }

  std::string get_topic() const { return trader_cmd_topic_; }

 private:
  RedisSession redis_;

  std::string account_abbreviation_;
  std::string trader_cmd_topic_;
};

}  // namespace ft

#endif  // FT_INCLUDE_IPC_REDIS_TRADER_CMD_HELPER_H_
