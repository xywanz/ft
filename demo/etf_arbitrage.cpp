// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include <fmt/format.h>

#include "strategy/order_sender.h"
#include "trading_server/risk_management/etf/etf_table.h"
#include "utils/redis.h"

using namespace ft;

const char* strategy_id = "etf_arb";

int wait_for_receipt(RedisSession* redis, int volume) {
  for (;;) {
    auto reply = redis->get_sub_reply();
    if (reply) {
      if (strcmp(reply->element[1]->str, strategy_id) == 0) {
        auto rsp = reinterpret_cast<const OrderResponse*>(reply->element[2]->str);
        auto contract = ContractTable::get_by_index(rsp->tid);
        spdlog::info("rsp: {} {} {}{} {}/{} traded:{}, price:{:.3f} completed:{}",
                     rsp->client_order_id, contract->ticker, DirectionToStr(rsp->direction),
                     OffsetToStr(rsp->offset), rsp->traded_volume, rsp->original_volume,
                     rsp->this_traded, rsp->this_traded_price, rsp->completed);
        if (rsp->completed) return volume - rsp->traded_volume;
      }
    }
  }
}

int main() {
  ContractTable::Init("../config/xtp_contracts.csv");
  spdlog::set_level(spdlog::level::from_str("debug"));
  bool init_res = EtfTable::Init("../config/etf_list.csv", "../config/etf_components.csv");
  spdlog::info("init_res: {}", init_res);

  auto etf = EtfTable::get_by_ticker("159901");
  RedisSession redis;
  OrderSender sender;

  sender.set_account(5319);
  sender.set_id(strategy_id);
  redis.subscribe({strategy_id});

  uint32_t client_order_id = 1;
  int left;

  for (const auto& [tid, component] : etf->components) {
    UNUSED(tid);
    left = component.volume;
    for (;;) {
      sender.SendOrder(component.contract->ticker, left, Direction::BUY, Offset::OPEN,
                       OrderType::MARKET, 0, client_order_id);
      left = wait_for_receipt(&redis, left);
      if (left == 0) break;
    }

    ++client_order_id;
    usleep(50000);
  }

  ++client_order_id;
  left = etf->unit;
  for (;;) {
    sender.SendOrder(etf->contract->ticker, left, Direction::PURCHASE, Offset::OPEN,
                     OrderType::LIMIT, 0, client_order_id);
    left = wait_for_receipt(&redis, left);
    if (left == 0) break;
  }

  ++client_order_id;
  left = etf->unit;
  for (;;) {
    sender.SendOrder(etf->contract->ticker, left, Direction::SELL, Offset::CLOSE_YESTERDAY,
                     OrderType::MARKET, 0, client_order_id);
    left = wait_for_receipt(&redis, left);
    if (left == 0) break;
  }

  // 下面是暴力发单的方式，之前压力测试把XTP测试服务器测崩了，好像被XTP禁了
  // for (const auto& [tid, component] : etf->components) {
  //   UNUSED(tid);
  //   sender.send_order(component.contract->ticker, component.volume,
  //                     Direction::BUY, Offset::OPEN, OrderType::MARKET, 0,
  //                     client_order_id);

  //   ++client_order_id;
  // }

  // uint32_t count = 0;
  // for (;;) {
  //   auto reply = redis.get_sub_reply();
  //   if (reply) {
  //     if (strcmp(reply->element[1]->str, strategy_id) == 0) {
  //       auto rsp =
  //           reinterpret_cast<const OrderResponse*>(reply->element[2]->str);
  //       auto contract = ContractTable::get_by_index(rsp->tid);
  //       spdlog::info(
  //           "rsp: {} {} {}{} {}/{} traded:{}, price:{:.3f} completed:{}",
  //           rsp->client_order_id, contract->ticker,
  //           direction_str(rsp->direction), offset_str(rsp->offset),
  //           rsp->traded_volume, rsp->original_volume, rsp->this_traded,
  //           rsp->this_traded_price, rsp->completed);
  //       if (rsp->completed) {
  //         left = rsp->original_volume - rsp->traded_volume;
  //         if (left == 0) {
  //           ++count;
  //           if (count == etf->components.size()) break;
  //         } else {
  //           sender.send_order(contract->ticker,
  //                             rsp->original_volume - rsp->traded_volume,
  //                             Direction::BUY, Offset::OPEN,
  //                             OrderType::MARKET, 0, rsp->client_order_id);
  //         }
  //       }
  //     }
  //   }
  // }

  // ++client_order_id;
  // left = etf->unit;
  // for (;;) {
  //   sender.send_order(etf->contract->ticker, left, Direction::PURCHASE,
  //                     Offset::OPEN, OrderType::LIMIT, 0, client_order_id);
  //   left = wait_for_receipt(&redis, left);
  //   if (left == 0) break;
  // }

  // ++client_order_id;
  // left = etf->unit;
  // for (;;) {
  //   sender.send_order(etf->contract->ticker, left, Direction::SELL,
  //                     Offset::CLOSE_YESTERDAY, OrderType::MARKET, 0,
  //                     client_order_id);
  //   left = wait_for_receipt(&redis, left);
  //   if (left == 0) break;
  // }

  spdlog::info("premium arbitrage of etf is done");
}
