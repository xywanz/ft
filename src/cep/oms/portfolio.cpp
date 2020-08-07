// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "cep/oms/portfolio.h"

#include <spdlog/spdlog.h>

#include <algorithm>

#include "cep/data/constants.h"
#include "cep/data/contract_table.h"

namespace ft {

Portfolio::Portfolio(bool sync_to_redis) {
  if (sync_to_redis) redis_ = std::make_unique<RedisPositionSetter>();
}

void Portfolio::set_account(uint64_t account) {
  if (redis_) {
    redis_->set_account(account);
    redis_->clear();
  }
}

void Portfolio::set_position(const Position& pos) {
  pos_map_.emplace(pos.tid, pos);

  if (redis_) {
    auto contract = ContractTable::get_by_index(pos.tid);
    assert(contract);
    redis_->set(contract->ticker, pos);
  }
}

void Portfolio::update_pending(uint32_t tid, uint32_t direction,
                               uint32_t offset, int changed) {
  if (changed == 0) return;

  if (direction == Direction::BUY || direction == Direction::SELL) {
    update_buy_or_sell_pending(tid, direction, offset, changed);
  } else if (direction == Direction::PURCHASE ||
             direction == Direction::REDEEM) {
    update_purchase_or_redeem_pending(tid, direction, changed);
  }
}

void Portfolio::update_buy_or_sell_pending(uint32_t tid, uint32_t direction,
                                           uint32_t offset, int changed) {
  bool is_close = is_offset_close(offset);
  if (is_close) direction = opp_direction(direction);

  auto& pos = find_or_create_pos(tid);
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

  // const auto* contract = ContractTable::get_by_index(pos.tid);
  // assert(contract);
  // redis_.set(proto_.pos_key(contract->ticker), &pos, sizeof(pos));
}

void Portfolio::update_purchase_or_redeem_pending(uint32_t tid,
                                                  uint32_t direction,
                                                  int changed) {
  auto& pos = find_or_create_pos(tid);
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

  // const auto* contract = ContractTable::get_by_index(pos.tid);
  // assert(contract);
  // redis_.set(proto_.pos_key(contract->ticker), &pos, sizeof(pos));
}

void Portfolio::update_traded(uint32_t tid, uint32_t direction, uint32_t offset,
                              int traded, double traded_price) {
  if (traded <= 0) return;

  if (direction == Direction::BUY || direction == Direction::SELL) {
    update_buy_or_sell(tid, direction, offset, traded, traded_price);
  } else if (direction == Direction::PURCHASE ||
             direction == Direction::REDEEM) {
    update_purchase_or_redeem(tid, direction, traded);
  }
}

void Portfolio::update_buy_or_sell(uint32_t tid, uint32_t direction,
                                   uint32_t offset, int traded,
                                   double traded_price) {
  bool is_close = is_offset_close(offset);
  if (is_close) direction = opp_direction(direction);

  auto& pos = find_or_create_pos(tid);
  auto& pos_detail = direction == Direction::BUY ? pos.long_pos : pos.short_pos;

  if (is_close) {
    pos_detail.close_pending -= traded;
    pos_detail.holdings -= traded;
    // 这里close_yesterday也执行这个操作是为了防止有些交易所不区分昨今仓，
    // 但用户平仓的时候却使用了close_yesterday
    if (offset == Offset::CLOSE_YESTERDAY || offset == Offset::CLOSE)
      pos_detail.yd_holdings -= std::min(pos_detail.yd_holdings, traded);

    if (pos_detail.holdings < pos_detail.yd_holdings) {
      spdlog::warn("yd pos fixed");
      pos_detail.yd_holdings = pos_detail.holdings;
    }
  } else {
    pos_detail.open_pending -= traded;
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

  const auto* contract = ContractTable::get_by_index(tid);
  if (!contract) {
    spdlog::error("[Position::update_buy_or_sell] Contract not found");
    return;
  }
  assert(contract->size > 0);

  // 如果是开仓则计算当前持仓的成本价
  if (is_offset_open(offset) && pos_detail.holdings > 0) {
    double cost = contract->size * (pos_detail.holdings - traded) *
                      pos_detail.cost_price +
                  contract->size * traded * traded_price;
    pos_detail.cost_price = cost / (pos_detail.holdings * contract->size);
  }

  if (pos_detail.holdings == 0) {
    pos_detail.cost_price = 0;
  }

  if (redis_) redis_->set(contract->ticker, pos);
}

void Portfolio::update_purchase_or_redeem(uint32_t tid, uint32_t direction,
                                          int traded) {
  auto& pos = find_or_create_pos(tid);
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

  if (redis_) {
    const auto* contract = ContractTable::get_by_index(tid);
    if (!contract) {
      spdlog::error("[Position::update_purchase_or_redeem] Contract not found");
      return;
    }
    redis_->set(contract->ticker, pos);
  }
}

void Portfolio::update_component_stock(uint32_t tid, int traded, bool acquire) {
  auto& pos = find_or_create_pos(tid);
  auto& pos_detail = pos.long_pos;

  if (acquire) {
    pos_detail.holdings += traded;
    pos_detail.yd_holdings += traded;
  } else {
    int td_pos = pos_detail.holdings - pos_detail.yd_holdings;
    pos_detail.holdings -= traded;
    pos_detail.yd_holdings -= std::max(traded - td_pos, 0);
  }

  if (redis_) {
    const auto* contract = ContractTable::get_by_index(tid);
    if (!contract) {
      spdlog::error("[Position::update_purchase_or_redeem] Contract not found");
      return;
    }
    redis_->set(contract->ticker, pos);
  }
}

void Portfolio::update_float_pnl(uint32_t tid, double last_price) {
  auto* pos = find(tid);
  if (pos) {
    const auto* contract = ContractTable::get_by_index(tid);
    if (!contract || contract->size <= 0) return;

    auto& lp = pos->long_pos;
    auto& sp = pos->short_pos;

    if (lp.holdings > 0)
      lp.float_pnl =
          lp.holdings * contract->size * (last_price - lp.cost_price);

    if (sp.holdings > 0)
      sp.float_pnl =
          sp.holdings * contract->size * (sp.cost_price - last_price);

    if (redis_) {
      if (lp.holdings > 0 || sp.holdings > 0) {
        redis_->set(contract->ticker, *pos);
      }
    }
  }
}

void Portfolio::update_on_query_trade(uint32_t tid, uint32_t direction,
                                      uint32_t offset, int closed_volume) {
  // auto* pos = find(tid);
  // if (!pos) return;

  // const auto* contract = ContractTable::get_by_index(tid);
  // if (!contract) return;

  // redis_.set(proto_.pos_key(contract->ticker), pos, sizeof(*pos));
}

}  // namespace ft
