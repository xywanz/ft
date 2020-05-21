// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "Strategy/Strategy.h"

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
    topics.emplace_back(proto_md_topic(ticker));
  tick_redis_.subscribe(topics);
}

void Strategy::send_order(const std::string& ticker, int volume,
                          uint32_t direction, uint32_t offset, uint32_t type,
                          double price, uint32_t user_order_id) {
  spdlog::info(
      "[Strategy::send_order] ticker: {}, volume: {}, price: {}, "
      "type: {}, direction: {}, offset: {}",
      ticker, volume, price, ordertype_str(type), direction_str(direction),
      offset_str(offset));

  const Contract* contract;
  contract = ContractTable::get_by_ticker(ticker);
  assert(contract);

  TraderCommand cmd{};
  cmd.magic = TRADER_CMD_MAGIC;
  cmd.type = NEW_ORDER;
  strncpy(cmd.strategy_id, strategy_id_, sizeof(cmd.strategy_id));
  cmd.order_req.user_order_id = user_order_id;
  cmd.order_req.ticker_index = contract->index;
  cmd.order_req.volume = volume;
  cmd.order_req.direction = direction;
  cmd.order_req.offset = offset;
  cmd.order_req.type = type;
  cmd.order_req.price = price;

  cmd_redis_.publish(TRADER_CMD_TOPIC, &cmd, sizeof(cmd));
}

}  // namespace ft
