// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_INCLUDE_POSITION_H_
#define FT_INCLUDE_POSITION_H_

#include <atomic>
#include <map>
#include <string>
#include <vector>

#include <spdlog/spdlog.h>

#include "Contract.h"
#include "Common.h"

namespace ft {

struct PositionDetail {
  PositionDetail() {}

  PositionDetail(const PositionDetail& other)
    : yd_volume(other.yd_volume),
      volume(other.volume),
      frozen(other.frozen),
      open_pending(other.open_pending.load()),
      close_pending(other.close_pending.load()),
      cost_price(other.cost_price),
      pnl(other.pnl) {}

  int64_t yd_volume = 0;
  int64_t volume = 0;
  int64_t frozen = 0;
  std::atomic<int64_t> open_pending = 0;
  std::atomic<int64_t> close_pending = 0;
  double cost_price = 0;
  double pnl = 0;
};

struct Position {
  Position() {}

  explicit Position(const std::string& _ticker)
    : ticker(_ticker) {
    ticker_split(ticker, &symbol, &exchange);
  }

  Position(const std::string& _symbol, const std::string& _exchange)
    : symbol(_symbol),
      exchange(_exchange),
      ticker(to_ticker(_symbol, _exchange)) {}

  Position(const Position& other)
    : symbol(other.symbol),
      exchange(other.exchange),
      ticker(other.ticker),
      long_pos(other.long_pos),
      short_pos(other.short_pos) {}

  void update_volume(Direction direction, Offset offset,
                     int traded, int pending_changed, double traded_price = 0.0) {
    bool is_close = is_offset_close(offset);
    if (is_close)
      direction = opp_direction(direction);

    auto& pos_detail = direction == Direction::BUY ? long_pos : short_pos;

    if (offset == Offset::OPEN) {
      pos_detail.open_pending += pending_changed;
      pos_detail.volume += traded;
    } else if (is_close) {
      pos_detail.close_pending += pending_changed;
      pos_detail.volume -= traded;
    }

    // TODO(kevin): 这里可能出问题
    // 如果abort可能是trade在position之前到达，正常使用不可能出现
    // 如果close_pending小于0，也有可能是之前启动时的挂单成交了，
    // 这次重启时未重启获取挂单数量导致的
    assert(pos_detail.volume >= 0);
    assert(pos_detail.open_pending >= 0);
    assert(pos_detail.close_pending >= 0);

    if (traded == 0 || is_equal(traded_price, 0.0))
      return;

    const auto* contract = ContractTable::get_by_ticker(ticker);
    if (!contract) {
      spdlog::error("[Position::update_volume] on_trade. Contract not found. Ticker: {}", ticker);
      return;
    }

    double cost = contract->size * (pos_detail.volume - traded) * pos_detail.cost_price;
    if (is_close)
      cost -= contract->size * traded * traded_price;
    else
      cost += contract->size * traded * traded_price;

    if (pos_detail.volume > 0 && contract->size > 0)
      pos_detail.cost_price = cost / (pos_detail.volume * contract->size);
    else
      pos_detail.cost_price = 0;
  }

  void update_pnl(double last_price) {
    auto contract = ContractTable::get_by_ticker(ticker);
    if (!contract || contract->size <= 0)
      return;

    if (long_pos.volume > 0)
      long_pos.pnl = long_pos.volume * contract->size * (last_price - long_pos.cost_price);

    if (short_pos.volume > 0)
      short_pos.pnl = short_pos.volume * contract->size * (short_pos.cost_price - last_price);
  }

  std::string symbol;
  std::string exchange;
  std::string ticker;

  PositionDetail long_pos;
  PositionDetail short_pos;

  int64_t short_yd_volume = 0;
  int64_t short_volume = 0;
  int64_t short_frozen = 0;
  std::atomic<int64_t> short_open_pending = 0;
  std::atomic<int64_t> short_close_pending = 0;
  double short_cost_price = 0;
  double short_pnl = 0;
};

/*
 * 不是线程安全的，对于更新来说是安全的，因为更新都是单线程的，
 * 但是get_position不安全，因为get_position的同时，可能会有
 * 更新线程在执行任务
 */
class PositionManager {
 public:
  PositionManager() {}

  void init_position(const Position& pos) {
    auto iter = pos_map_.find(pos.ticker);
    if (iter != pos_map_.end()) {
      spdlog::warn("[PositionManager::init_position] Failed to init pos: position already exists");
      return;
    }

    pos_map_.emplace(pos.ticker, pos);
  }

  void update_volume(const std::string& ticker, Direction direction, Offset offset,
                     int traded, int pending_changed, double traded_price = 0.0) {
    if (traded == 0 && pending_changed == 0)
      return;

    auto& pos = find_or_create_pos(ticker);
    pos.update_volume(direction, offset, traded, pending_changed, traded_price);
  }

  void update_pnl(const std::string& ticker, double last_price) {
    auto iter = pos_map_.find(ticker);
    if (iter == pos_map_.end())
      return;

    iter->second.update_pnl(last_price);
  }

  const Position* get_position(const std::string& ticker) const {
    auto iter = pos_map_.find(ticker);
    if (iter == pos_map_.end())
      return nullptr;
    return &iter->second;
  }

  void clear() {
    pos_map_.clear();
  }

 private:
  Position& find_or_create_pos(const std::string& ticker) {
    auto& pos = pos_map_[ticker];
    if (pos.ticker.empty()) {
      pos.ticker = ticker;
      ticker_split(pos.ticker, &pos.symbol, &pos.exchange);
    }
    return pos;
  }

 private:
  std::map<std::string, Position> pos_map_;
};

}  // namespace ft

#endif  // FT_INCLUDE_POSITION_H_
