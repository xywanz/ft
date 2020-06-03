// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "strategy/strategy.h"

namespace ft {

void Strategy::run() {
  on_init();
  redis_.subscribe({strategy_id_});

  for (;;) {
    auto reply = redis_.get_sub_reply();
    if (reply) {
      if (strcmp(reply->element[1]->str, strategy_id_) == 0) {
        auto rsp =
            reinterpret_cast<const OrderResponse*>(reply->element[2]->str);
        on_order_rsp(*rsp);
      } else {
        auto tick = reinterpret_cast<const TickData*>(reply->element[2]->str);
        on_tick(*tick);
      }
    }
  }
}

void Strategy::subscribe(const std::vector<std::string>& sub_list) {
  std::vector<std::string> topics;
  for (const auto& ticker : sub_list)
    topics.emplace_back(proto_.quote_key(ticker));
  redis_.subscribe(topics);
}

}  // namespace ft
