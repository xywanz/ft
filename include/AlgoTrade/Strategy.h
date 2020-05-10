// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_INCLUDE_ALGOTRADE_STRATEGY_H_
#define FT_INCLUDE_ALGOTRADE_STRATEGY_H_

#include <memory>
#include <string>
#include <vector>

#include "AlgoTrade/Context.h"
#include "Base/DataStruct.h"
#include "IPC/redis.h"

namespace ft {

class Strategy {
 public:
  Strategy() : redis_tick_("127.0.0.1", 6379) {}

  virtual ~Strategy() {}

  void subscribe(const std::vector<std::string>& sub_list) {
    std::vector<std::string> topics;
    for (const auto& ticker : sub_list)
      topics.emplace_back(fmt::format("md-{}", ticker));
    redis_tick_.subscribe(topics);
  }

  virtual void on_init(AlgoTradeContext* ctx) {}

  virtual void on_tick(AlgoTradeContext* ctx, const TickData* tick) {}

  virtual void on_order(AlgoTradeContext* ctx, const Order* order) {}

  virtual void on_trade(AlgoTradeContext* ctx, const Trade* trade) {}

  virtual void on_exit(AlgoTradeContext* ctx) {}

  void run() {
    on_init(&ctx_);
    for (;;) {
      auto reply = redis_tick_.get_sub_reply();
      auto tick = reinterpret_cast<const TickData*>(reply->element[2]->str);
      on_tick(&ctx_, tick);
    }
  }

 private:
  AlgoTradeContext ctx_;
  RedisSession redis_tick_;
};

#define EXPORT_STRATEGY(type) \
  extern "C" ft::Strategy* create_strategy() { return new type; }

}  // namespace ft

#endif  // FT_INCLUDE_ALGOTRADE_STRATEGY_H_
