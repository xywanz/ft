// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "Base/Portfolio.h"

#include "ContractTable.h"

namespace ft {

void Portfolio::init_position(const Position& pos) {
  if (find(pos.ticker_index)) {
    spdlog::warn(
        "[Portfolio::init_position] Failed to init pos: position already "
        "exists");
    return;
  }

  pos_map_.emplace(pos.ticker_index, pos);
  spdlog::debug("[Portfolio::init_position]");
}

void Portfolio::update_pending(uint64_t ticker_index, Direction direction,
                               Offset offset, int changed) {
  if (changed == 0) return;

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
    spdlog::warn("[Portfolio::update_pending] correct open_pending");
  }

  if (pos_detail.close_pending < 0) {
    pos_detail.close_pending = 0;
    spdlog::warn("[Portfolio::update_pending] correct close_pending");
  }
}

void Portfolio::update_traded(uint64_t ticker_index, Direction direction,
                              Offset offset, int64_t traded,
                              double traded_price) {
  if (traded <= 0) return;

  bool is_close = is_offset_close(offset);
  if (is_close) direction = opp_direction(direction);

  auto& pos = find_or_create_pos(ticker_index);
  auto& pos_detail = direction == Direction::BUY ? pos.long_pos : pos.short_pos;
  if (is_close) {
    pos_detail.close_pending -= traded;
    pos_detail.volume -= traded;
  } else {
    pos_detail.open_pending -= traded;
    pos_detail.volume += traded;
  }

  // TODO(kevin): 这里可能出问题
  // 如果abort可能是trade在position之前到达，正常使用不可能出现
  // 如果close_pending小于0，也有可能是之前启动时的挂单成交了，
  // 这次重启时未重启获取挂单数量导致的
  assert(pos_detail.volume >= 0);

  if (pos_detail.open_pending < 0) {
    pos_detail.open_pending = 0;
    spdlog::warn("[Portfolio::update_traded] correct open_pending");
  }

  if (pos_detail.close_pending < 0) {
    pos_detail.close_pending = 0;
    spdlog::warn("[Portfolio::update_traded] correct close_pending");
  }

  const auto* contract = ContractTable::get_by_index(ticker_index);
  if (!contract) {
    spdlog::error("[Position::update_traded] Contract not found");
    return;
  }

  if (contract->size <= 0) {
    pos_detail.cost_price = 0;
    pos_detail.float_pnl = 0;
    return;
  }

  if (is_close) {  // 如果是平仓则计算已实现的盈亏
    if (direction == Direction::BUY)
      realized_pnl_ =
          contract->size * traded * (traded_price - pos_detail.cost_price);
    else
      realized_pnl_ =
          contract->size * traded * (pos_detail.cost_price - traded_price);
  } else if (pos_detail.volume > 0) {  // 如果是开仓则计算当前持仓的成本价
    double cost =
        contract->size * (pos_detail.volume - traded) * pos_detail.cost_price +
        contract->size * traded * traded_price;
    pos_detail.cost_price = cost / (pos_detail.volume * contract->size);
  }

  if (pos_detail.volume == 0) {
    pos_detail.float_pnl = 0;
    pos_detail.cost_price = 0;
  }
}

void Portfolio::update_float_pnl(uint64_t ticker_index, double last_price) {
  auto* pos = find(ticker_index);
  if (pos) {
    const auto* contract = ContractTable::get_by_index(ticker_index);
    if (!contract || contract->size <= 0) return;

    auto& lp = pos->long_pos;
    auto& sp = pos->short_pos;

    if (lp.volume > 0)
      lp.float_pnl = lp.volume * contract->size * (last_price - lp.cost_price);

    if (sp.volume > 0)
      sp.float_pnl = sp.volume * contract->size * (sp.cost_price - last_price);
  }
}

}  // namespace ft
