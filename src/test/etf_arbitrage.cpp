#include <fmt/format.h>

#include "common/order_sender.h"
#include "ipc/redis.h"
#include "risk_management/etf/etf_table.h"

using namespace ft;

int main() {
  const std::string strategy_id = "etf_arb";
  ContractTable::init("../config/xtp_contracts.csv");
  spdlog::set_level(spdlog::level::from_str("debug"));

  bool init_res =
      EtfTable::init("../config/etf_list.csv", "../config/etf_components.csv");
  spdlog::info("init_res: {}", init_res);

  auto etf = EtfTable::get_by_ticker("159901");
  RedisSession redis;
  OrderSender sender;

  sender.set_account(5319);
  sender.set_id(strategy_id);
  redis.subscribe({strategy_id});

  // std::thread([&] {
  //   for (;;) {
  //     auto reply = redis.get_sub_reply();
  //     if (reply) {
  //       if (strcmp(reply->element[1]->str, strategy_id.c_str()) == 0) {
  //         auto rsp =
  //             reinterpret_cast<const OrderResponse*>(reply->element[2]->str);
  //         auto contract = ContractTable::get_by_index(rsp->ticker_index);
  //         spdlog::info("rsp: {} {}/{} completed:{}", contract->ticker,
  //                      rsp->traded_volume, rsp->original_volume,
  //                      rsp->completed);
  //       }
  //     }
  //   }
  // }).detach();

  // sender.send_order("159901", 10000, Direction::BUY, Offset::OPEN,
  //                   OrderType::MARKET, 0, 0);
  // for (;;) {
  // }

  uint32_t user_order_id = 1;
  for (const auto& [ticker_index, component] : etf->components) {
    UNUSED(ticker_index);
    int left = component.volume;
    sender.send_order(component.contract->ticker, component.volume,
                      Direction::BUY, Offset::OPEN, OrderType::MARKET, 0,
                      user_order_id);
    for (;;) {
      auto reply = redis.get_sub_reply();
      if (reply) {
        if (strcmp(reply->element[1]->str, strategy_id.c_str()) == 0) {
          auto rsp =
              reinterpret_cast<const OrderResponse*>(reply->element[2]->str);
          spdlog::info("rsp: {} {} {}/{} completed:{}", rsp->user_order_id,
                       component.contract->ticker, rsp->traded_volume,
                       rsp->original_volume, rsp->completed);
          if (rsp->completed == true) {
            left -= rsp->traded_volume;
            if (left == 0) break;
            sender.send_order(component.contract->ticker, left, Direction::BUY,
                              Offset::OPEN, OrderType::MARKET, 0,
                              user_order_id);
          }
        }
      }
    }

    ++user_order_id;
    usleep(100000);
  }

  ++user_order_id;
  sender.send_order(etf->contract->ticker, etf->unit, Direction::PURCHASE,
                    Offset::OPEN, OrderType::LIMIT, 0, user_order_id);
  for (;;) {
    auto reply = redis.get_sub_reply();
    if (reply) {
      if (strcmp(reply->element[1]->str, strategy_id.c_str()) == 0) {
        auto rsp =
            reinterpret_cast<const OrderResponse*>(reply->element[2]->str);
        spdlog::info("rsp: {} {} {}/{} completed:{}", rsp->user_order_id,
                     etf->contract->ticker, rsp->traded_volume,
                     rsp->original_volume, rsp->completed);
        if (rsp->completed == true) break;
      }
    }
  }

  ++user_order_id;
  int left = etf->unit;
  sender.send_order(etf->contract->ticker, etf->unit, Direction::SELL,
                    Offset::CLOSE_YESTERDAY, OrderType::MARKET, 0,
                    user_order_id);
  for (;;) {
    auto reply = redis.get_sub_reply();
    if (reply) {
      if (strcmp(reply->element[1]->str, strategy_id.c_str()) == 0) {
        auto rsp =
            reinterpret_cast<const OrderResponse*>(reply->element[2]->str);
        spdlog::info("rsp: {} {} {}/{} completed:{}", rsp->user_order_id,
                     etf->contract->ticker, rsp->traded_volume,
                     rsp->original_volume, rsp->completed);
        if (rsp->completed == true) {
          left -= rsp->traded_volume;
          if (left == 0) break;
          sender.send_order(etf->contract->ticker, left, Direction::SELL,
                            Offset::CLOSE_YESTERDAY, OrderType::MARKET, 0,
                            user_order_id);
        }
      }
    }
  }

  spdlog::info("done");
}
