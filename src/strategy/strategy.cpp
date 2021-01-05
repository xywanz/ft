// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "strategy/strategy.h"

namespace ft {

void Strategy::Run() {
  OnInit();
  puller_.SubscribeOrderResponse(strategy_id_);

  for (;;) {
    auto reply = puller_.Pull();
    if (reply) {
      if (strcmp(reply->element[1]->str, strategy_id_) == 0) {
        auto rsp = reinterpret_cast<const OrderResponse*>(reply->element[2]->str);
        OnOrderResponse(*rsp);
      } else {
        auto tick = reinterpret_cast<TickData*>(reply->element[2]->str);
        OnTick(*tick);
      }
    }
  }
}

void Strategy::Subscribe(const std::vector<std::string>& sub_list) {
  puller_.SubscribeMarketData(sub_list);
}

}  // namespace ft
