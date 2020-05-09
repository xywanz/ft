// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "PositionManager.h"

#include "ContractTable.h"

namespace ft {

PositionManager::PositionManager(const std::string& ip, int port)
    : redis_sess_(ip, port) {}

Position PositionManager::get_position(const std::string& ticker) const {
  static const Position empty_pos{};

  auto reply = redis_sess_.get(get_pos_key(ticker));

  if (reply->len == 0)
    return empty_pos;

  assert(reply->len == sizeof(Position));
  Position pos;
  memcpy(&pos, reply->str, reply->len);
  return pos;
}

double PositionManager::get_realized_pnl() const {
  auto reply = redis_sess_.get(get_pnl_key());
  if (reply->len == 0)
    return 0;

  assert(reply->len == sizeof(realized_pnl_));
  double ret = *reinterpret_cast<double*>(reply->str);
  return ret;
}

double PositionManager::get_float_pnl() const {
  double float_pnl = 0;
  auto reply = redis_sess_.keys("pos-*");
  for (size_t i = 0; i < reply->elements; ++i) {
    auto key = reinterpret_cast<const char*>(reply->element[i]->str);
    auto pos_reply = redis_sess_.get(key);
    auto pos = reinterpret_cast<const Position*>(pos_reply->str);
    float_pnl += pos->long_pos.float_pnl + pos->short_pos.float_pnl;
  }
  return float_pnl;
}

void PositionManager::set_position(const Position* pos) {
  pos_map_.emplace(pos->ticker_index, *pos);

  const auto* contract = ContractTable::get_by_index(pos->ticker_index);
  assert(contract);
  auto key = get_pos_key(contract->ticker);
  redis_sess_.set(key, pos, sizeof(Position));
}

void PositionManager::update_pending(uint64_t ticker_index, Direction direction,
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

  const auto* contract = ContractTable::get_by_index(pos.ticker_index);
  assert(contract);
  redis_sess_.set(get_pos_key(contract->ticker), &pos, sizeof(pos));
}

void PositionManager::update_traded(uint64_t ticker_index, Direction direction,
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
  assert(contract->size > 0);

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

  redis_sess_.set(get_pos_key(contract->ticker), &pos, sizeof(pos));
  redis_sess_.set(get_pnl_key(), &realized_pnl_, sizeof(realized_pnl_));
}

void PositionManager::update_float_pnl(uint64_t ticker_index,
                                       double last_price) {
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
