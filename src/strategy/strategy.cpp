// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "strategy/strategy.h"

namespace ft {

void Strategy::run() {
  on_init();

  std::thread rsp_receiver([this] {
    rsp_redis_.subscribe({strategy_id_});

    for (;;) {
      auto reply = rsp_redis_.get_sub_reply();
      if (reply) {
        auto rsp =
            reinterpret_cast<const OrderResponse*>(reply->element[2]->str);
        on_order_rsp(rsp);
      }
    }
  });

  for (;;) {
    auto reply = tick_redis_.get_sub_reply();
    if (reply) {
      auto tick = reinterpret_cast<const TickData*>(reply->element[2]->str);
      on_tick(tick);
    }
  }
}

void Strategy::subscribe(const std::vector<std::string>& sub_list) {
  std::vector<std::string> topics;
  for (const auto& ticker : sub_list)
    topics.emplace_back(proto_.quote_key(ticker));
  tick_redis_.subscribe(topics);
}

}  // namespace ft
