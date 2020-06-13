// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "common/portfolio.h"

#include <algorithm>

#include "core/constants.h"
#include "core/contract_table.h"

namespace ft {

Portfolio::Portfolio(const std::string& ip, int port) : redis_(ip, port) {}

void Portfolio::init(uint64_t account) {
  proto_.set_account(account);
  auto reply = redis_.keys(fmt::format("{}*", proto_.pos_key_prefix()));
  for (size_t i = 0; i < reply->elements; ++i)
    redis_.del(reply->element[i]->str);
}

void Portfolio::set_position(const Position& pos) {
  pos_map_.emplace(pos.ticker_index, pos);

  const auto* contract = ContractTable::get_by_index(pos.ticker_index);
  assert(contract);
  auto key = proto_.pos_key(contract->ticker);
  redis_.set(key, &pos, sizeof(Position));
}

void Portfolio::update_pending(uint32_t ticker_index, uint32_t direction,
                               uint32_t offset, int changed) {
  if (changed == 0) return;

  if (direction == Direction::BUY || direction == Direction::SELL) {
    update_buy_or_sell_pending(ticker_index, direction, offset, changed);
  } else if (direction == Direction::PURCHASE ||
             direction == Direction::REDEEM) {
    update_purchase_or_redeem_pending(ticker_index, direction, changed);
  }
}

void Portfolio::update_buy_or_sell_pending(uint32_t ticker_index,
                                           uint32_t direction, uint32_t offset,
                                           int changed) {
  bool is_close = is_offset_close(offset);
  if (is_close) direction = opp_direction(direction);

  auto& pos = find_or_create_pos(ticker_index);
  auto& pos_detail = direction == Direction::BUY ? pos.long_pos : pos.short_pos;
  if (is_close)
    pos_detail.close_pending += changed;
  else
    pos_detail.open_pending += changed;

  if (pos_detail.open_pending < 0) {
    pos_detail.open_pending = 0;
    // spdlog::warn("[Portfolio::update_buy_or_sell_pending] correct
    // open_pending");
  }

  if (pos_detail.close_pending < 0) {
    pos_detail.close_pending = 0;
    // spdlog::warn("[Portfolio::update_buy_or_sell_pending] correct
    // close_pending");
  }

  const auto* contract = ContractTable::get_by_index(pos.ticker_index);
  assert(contract);
  redis_.set(proto_.pos_key(contract->ticker), &pos, sizeof(pos));
}

void Portfolio::update_purchase_or_redeem_pending(uint32_t ticker_index,
                                                  uint32_t direction,
                                                  int changed) {
  auto& pos = find_or_create_pos(ticker_index);
  auto& pos_detail = pos.long_pos;

  if (direction == Direction::PURCHASE) {
    pos_detail.open_pending += changed;
  } else {
    pos_detail.close_pending += changed;
  }

  if (pos_detail.open_pending < 0) {
    pos_detail.open_pending = 0;
    // spdlog::warn("[Portfolio::update_purchase_or_redeem_pending] correct
    // open_pending");
  }

  if (pos_detail.close_pending < 0) {
    pos_detail.close_pending = 0;
    // spdlog::warn("[Portfolio::update_purchase_or_redeem_pending] correct
    // close_pending");
  }

  const auto* contract = ContractTable::get_by_index(pos.ticker_index);
  assert(contract);
  redis_.set(proto_.pos_key(contract->ticker), &pos, sizeof(pos));
}

void Portfolio::update_traded(uint32_t ticker_index, uint32_t direction,
                              uint32_t offset, int traded, double traded_price,
                              bool to_update_pending) {
  if (traded <= 0) return;

  if (direction == Direction::BUY || direction == Direction::SELL) {
    update_buy_or_sell(ticker_index, direction, offset, traded, traded_price,
                       to_update_pending);
  } else if (direction == Direction::PURCHASE ||
             direction == Direction::REDEEM) {
    update_purchase_or_redeem(ticker_index, direction, traded);
  }
}

void Portfolio::update_buy_or_sell(uint32_t ticker_index, uint32_t direction,
                                   uint32_t offset, int traded,
                                   double traded_price,
                                   bool to_update_pending) {
  bool is_close = is_offset_close(offset);
  if (is_close) direction = opp_direction(direction);

  auto& pos = find_or_create_pos(ticker_index);
  auto& pos_detail = direction == Direction::BUY ? pos.long_pos : pos.short_pos;

  if (is_close) {
    if (to_update_pending) pos_detail.close_pending -= traded;
    pos_detail.holdings -= traded;
    // 这里close_yesterday也执行这个操作是为了防止有些交易所不区分昨今仓，
    // 但用户平仓的时候却使用了close_yesterday
    if (offset == Offset::CLOSE_YESTERDAY || offset == Offset::CLOSE)
      pos_detail.yd_holdings -= std::min(pos_detail.yd_holdings, traded);
    assert(pos_detail.yd_holdings >= 0);
    assert(pos_detail.holdings >= pos_detail.yd_holdings);
  } else {
    if (to_update_pending) pos_detail.open_pending -= traded;
    pos_detail.holdings += traded;
  }

  // TODO(kevin): 这里可能出问题
  // 如果abort可能是trade在position之前到达，正常使用不可能出现
  // 如果close_pending小于0，也有可能是之前启动时的挂单成交了，
  // 这次重启时未重启获取挂单数量导致的
  assert(pos_detail.holdings >= 0);

  if (pos_detail.open_pending < 0) {
    pos_detail.open_pending = 0;
    // spdlog::warn("[Portfolio::update_traded] correct open_pending");
  }

  if (pos_detail.close_pending < 0) {
    pos_detail.close_pending = 0;
    // spdlog::warn("[Portfolio::update_traded] correct close_pending");
  }

  const auto* contract = ContractTable::get_by_index(ticker_index);
  if (!contract) {
    spdlog::error("[Position::update_buy_or_sell] Contract not found");
    return;
  }
  assert(contract->size > 0);

  if (is_close) {  // 如果是平仓则计算已实现的盈亏
    if (direction == Direction::BUY)
      realized_pnl_ =
          contract->size * traded * (traded_price - pos_detail.cost_price);
    else
      realized_pnl_ =
          contract->size * traded * (pos_detail.cost_price - traded_price);
  } else if (pos_detail.holdings > 0) {  // 如果是开仓则计算当前持仓的成本价
    double cost = contract->size * (pos_detail.holdings - traded) *
                      pos_detail.cost_price +
                  contract->size * traded * traded_price;
    pos_detail.cost_price = cost / (pos_detail.holdings * contract->size);
  }

  if (pos_detail.holdings == 0) {
    pos_detail.float_pnl = 0;
    pos_detail.cost_price = 0;
  }

  redis_.set(proto_.pos_key(contract->ticker), &pos, sizeof(pos));
  redis_.set("realized_pnl", &realized_pnl_, sizeof(realized_pnl_));
}

void Portfolio::update_purchase_or_redeem(uint32_t ticker_index,
                                          uint32_t direction, int traded) {
  auto& pos = find_or_create_pos(ticker_index);
  auto& pos_detail = pos.long_pos;

  if (direction == Direction::PURCHASE) {
    pos_detail.open_pending -= traded;
    pos_detail.holdings += traded;
    pos_detail.yd_holdings += traded;
  } else {
    pos_detail.close_pending -= traded;

    int td_pos = pos_detail.holdings - pos_detail.yd_holdings;
    pos_detail.holdings -= traded;
    pos_detail.yd_holdings -= std::max(traded - td_pos, 0);

    if (pos_detail.holdings == 0) {
      pos_detail.float_pnl = 0;
      pos_detail.cost_price = 0;
    }
  }

  const auto* contract = ContractTable::get_by_index(ticker_index);
  if (!contract) {
    spdlog::error("[Position::update_purchase_or_redeem] Contract not found");
    return;
  }
  redis_.set(proto_.pos_key(contract->ticker), &pos, sizeof(pos));
}

void Portfolio::update_float_pnl(uint32_t ticker_index, double last_price) {
  auto* pos = find(ticker_index);
  if (pos) {
    const auto* contract = ContractTable::get_by_index(ticker_index);
    if (!contract || contract->size <= 0) return;

    auto& lp = pos->long_pos;
    auto& sp = pos->short_pos;

    if (lp.holdings > 0)
      lp.float_pnl =
          lp.holdings * contract->size * (last_price - lp.cost_price);

    if (sp.holdings > 0)
      sp.float_pnl =
          sp.holdings * contract->size * (sp.cost_price - last_price);

    if (lp.holdings > 0 || sp.holdings > 0)
      redis_.set(proto_.pos_key(contract->ticker), pos, sizeof(*pos));
  }
}

void Portfolio::update_on_query_trade(uint32_t ticker_index, uint32_t direction,
                                      uint32_t offset, int closed_volume) {
  // auto* pos = find(ticker_index);
  // if (!pos) return;

  // const auto* contract = ContractTable::get_by_index(ticker_index);
  // if (!contract) return;

  // redis_.set(proto_.pos_key(contract->ticker), pos, sizeof(*pos));
}

}  // namespace ft
