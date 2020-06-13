#include <fmt/format.h>

#include "common/order_sender.h"
#include "ipc/redis.h"
#include "risk_management/etf/etf_table.h"

using namespace ft;

const char* strategy_id = "etf_arb";

int wait_for_receipt(RedisSession* redis, int volume) {
  for (;;) {
    auto reply = redis->get_sub_reply();
    if (reply) {
      if (strcmp(reply->element[1]->str, strategy_id) == 0) {
        auto rsp =
            reinterpret_cast<const OrderResponse*>(reply->element[2]->str);
        auto contract = ContractTable::get_by_index(rsp->ticker_index);
        spdlog::info(
            "rsp: {} {} {}{} {}/{} traded:{}, price:{:.3f} completed:{}",
            rsp->user_order_id, contract->ticker, direction_str(rsp->direction),
            offset_str(rsp->offset), rsp->traded_volume, rsp->original_volume,
            rsp->this_traded, rsp->this_traded_price, rsp->completed);
        if (rsp->completed) return volume - rsp->traded_volume;
      }
    }
  }
}

int main() {
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

  uint32_t user_order_id = 1;
  int left;

  for (const auto& [ticker_index, component] : etf->components) {
    UNUSED(ticker_index);
    left = component.volume;
    for (;;) {
      sender.send_order(component.contract->ticker, left, Direction::BUY,
                        Offset::OPEN, OrderType::MARKET, 0, user_order_id);
      left = wait_for_receipt(&redis, left);
      if (left == 0) break;
    }

    ++user_order_id;
    usleep(50000);
  }

  ++user_order_id;
  left = etf->unit;
  for (;;) {
    sender.send_order(etf->contract->ticker, left, Direction::PURCHASE,
                      Offset::OPEN, OrderType::LIMIT, 0, user_order_id);
    left = wait_for_receipt(&redis, left);
    if (left == 0) break;
  }

  ++user_order_id;
  left = etf->unit;
  for (;;) {
    sender.send_order(etf->contract->ticker, left, Direction::SELL,
                      Offset::CLOSE_YESTERDAY, OrderType::MARKET, 0,
                      user_order_id);
    left = wait_for_receipt(&redis, left);
    if (left == 0) break;
  }

  spdlog::info("premium arbitrage of etf is done");
}
