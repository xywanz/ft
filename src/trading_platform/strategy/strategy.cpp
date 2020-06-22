// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "strategy/strategy.h"

namespace ft {

void Strategy::run() {
  on_init();
  puller_.subscribe_order_rsp(strategy_id_);

  for (;;) {
    auto reply = puller_.pull();
    if (reply) {
      if (strcmp(reply->element[1]->str, strategy_id_) == 0) {
        auto rsp =
            reinterpret_cast<const OrderResponse*>(reply->element[2]->str);
        on_order_rsp(*rsp);
      } else {
        auto tick = reinterpret_cast<TickData*>(reply->element[2]->str);
        on_tick(*tick);
      }
    }
  }
}

void Strategy::subscribe(const std::vector<std::string>& sub_list) {
  puller_.subscribe_md(sub_list);
}

}  // namespace ft
