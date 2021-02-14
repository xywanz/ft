// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "strategy/strategy.h"

#include <spdlog/spdlog.h>

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
        TickData tick;
        if (!tick.ParseFromString(reply->element[2]->str, reply->element[2]->len)) {
          spdlog::error("Strategy::Run: failed to parse tick data");
          continue;
        }
        OnTick(tick);
      }
    }
  }
}

void Strategy::RunBackTest() {
  OnInit();
  puller_.SubscribeOrderResponse(strategy_id_);
  SendNotification(0);  // 通知gateway开始发tick数据

  for (;;) {
    auto reply = puller_.Pull();
    if (reply) {
      if (strcmp(reply->element[1]->str, strategy_id_) == 0) {
        auto rsp = reinterpret_cast<const OrderResponse*>(reply->element[2]->str);
        OnOrderResponse(*rsp);
      } else {
        TickData tick;
        if (!tick.ParseFromString(reply->element[2]->str, reply->element[2]->len)) {
          spdlog::error("Strategy::Run: failed to parse tick data");
          abort();  // bug
        }
        OnTick(tick);
        SendNotification(0);  // 通知gateway数据已消费完
      }
    }
  }
}

void Strategy::Subscribe(const std::vector<std::string>& sub_list) {
  puller_.SubscribeMarketData(sub_list);
}

}  // namespace ft
