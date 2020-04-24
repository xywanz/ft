// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_INCLUDE_POSITION_H_
#define FT_INCLUDE_POSITION_H_

#include <atomic>
#include <map>
#include <mutex>
#include <string>
#include <vector>

#include <spdlog/spdlog.h>

#include "Contract.h"
#include "Common.h"

namespace ft {

struct PositionDetail {
  int64_t yd_volume = 0;
  int64_t volume = 0;
  int64_t frozen = 0;
  int64_t open_pending = 0;
  int64_t close_pending = 0;
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


  void update_pending(Direction direction, Offset offset, int changed) {
    if (changed == 0)
      return;

    bool is_close = is_offset_close(offset);
    if (is_close)
      direction = opp_direction(direction);

    auto& pos_detail = direction == Direction::BUY ? long_pos : short_pos;
    if (is_close)
      pos_detail.close_pending += changed;
    else
      pos_detail.open_pending += changed;

    assert(pos_detail.open_pending >= 0);
    assert(pos_detail.close_pending >= 0);
  }

  void update_traded(Direction direction, Offset offset, int traded, double traded_price) {
    if (traded == 0)
      return;

    bool is_close = is_offset_close(offset);
    if (is_close)
      direction = opp_direction(direction);

    auto& pos_detail = direction == Direction::BUY ? long_pos : short_pos;
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
    assert(pos_detail.open_pending >= 0);
    assert(pos_detail.close_pending >= 0);

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
};


/* 最新版本线程不安全！！！下面的评论无效，因PosMgr的函数都是小函数，故由外部保证线程安全
//  * 线程安全
//  * 虽说线程安全，使用上还是要注意，init_position要在其他任何仓位操作之前完成。
//  * 即启动时查询一次仓位，等待仓位全部初始化完毕后，才可进行交易，之后的仓位更新
//  * 操作全权交由PosManager负责，不再从API查询。
 */
class PositionManager {
 public:
  PositionManager() {}

  void init_position(const Position& pos) {
    if (find(pos.ticker)) {
      spdlog::warn("[PositionManager::init_position] Failed to init pos: position already exists");
      return;
    }

    pos_map_.emplace(pos.ticker, pos);
    spdlog::debug("[PositionManager::init_position]");
  }

  void update_pending(const std::string& ticker, Direction direction, Offset offset,
                      int changed) {
    if (changed == 0)
      return;

    auto& pos = find_or_create_pos(ticker);
    pos.update_pending(direction, offset, changed);
  }

  void update_traded(const std::string& ticker, Direction direction, Offset offset,
                     int traded, double traded_price) {
    if (traded == 0)
      return;

    auto& pos = find_or_create_pos(ticker);
    pos.update_traded(direction, offset, traded, traded_price);
  }

  void update_pnl(const std::string& ticker, double last_price) {
    auto* pos = find(ticker);
    if (pos)
      pos->update_pnl(last_price);
  }

  const Position* get_position_unsafe(const std::string& ticker) const {
    static const Position empty_pos;

    const auto* pos = find(ticker);
    return pos ? pos : &empty_pos;
  }

  void get_pos_ticker_list_unsafe(std::vector<const std::string*>* out) const {
    for (const auto& [ticker, pos] : pos_map_) {
      if (!is_empty_pos(pos))
        out->emplace_back(&ticker);
    }
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

  bool is_empty_pos(const Position& pos) const {
    const auto& lp = pos.long_pos;
    const auto& sp = pos.short_pos;
    return lp.open_pending == 0 && lp.close_pending == 0 && lp.frozen == 0 &&
           lp.volume == 0 && sp.open_pending == 0 && sp.close_pending == 0 &&
           sp.frozen == 0 && sp.volume == 0;
  }

  Position* find(const std::string& ticker) {
    auto iter = pos_map_.find(ticker);
    if (iter == pos_map_.end())
      return nullptr;
    return &iter->second;
  }

  const Position* find(const std::string& ticker) const {
    return const_cast<PositionManager*>(this)->find(ticker);
  }

 private:
  std::map<std::string, Position> pos_map_;
};

}  // namespace ft

#endif  // FT_INCLUDE_POSITION_H_
