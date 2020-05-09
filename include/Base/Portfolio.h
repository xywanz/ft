// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_INCLUDE_PORTFOLIO_H_
#define FT_INCLUDE_PORTFOLIO_H_

#include <spdlog/spdlog.h>

#include <map>
#include <string>
#include <vector>

#include "Base/Order.h"
#include "Base/Position.h"

namespace ft {

/* 最新版本线程不安全！！！下面的评论无效，因PosMgr的函数都是小函数，故由外部保证线程安全
//  * 线程安全
//  *
虽说线程安全，使用上还是要注意，init_position要在其他任何仓位操作之前完成。
//  *
即启动时查询一次仓位，等待仓位全部初始化完毕后，才可进行交易，之后的仓位更新
//  * 操作全权交由Portfolio负责，不再从API查询。
 */
class Portfolio {
 public:
  Portfolio() {}

  void init_position(const Position& pos);

  void update_pending(uint64_t ticker_index, Direction direction, Offset offset,
                      int changed);

  void update_traded(uint64_t ticker_index, Direction direction, Offset offset,
                     int64_t traded, double traded_price);

  void update_float_pnl(uint64_t ticker_index, double last_price);

  const Position* get_position_unsafe(uint64_t ticker_index) const {
    static const Position empty_pos{};

    const auto* pos = find(ticker_index);
    return pos ? pos : &empty_pos;
  }

  void get_pos_ticker_list(std::vector<uint64_t>* out) const {
    for (const auto& [ticker_index, pos] : pos_map_) {
      if (!is_empty_pos(pos)) out->emplace_back(ticker_index);
    }
  }

  double get_realized_pnl() const { return realized_pnl_; }

  double get_float_pnl() const {
    double float_pnl = 0;
    for (auto& pair : pos_map_)
      float_pnl +=
          pair.second.long_pos.float_pnl + pair.second.short_pos.float_pnl;
    return float_pnl;
  }

  void clear() { pos_map_.clear(); }

 private:
  Position& find_or_create_pos(uint64_t ticker_index) {
    auto& pos = pos_map_[ticker_index];
    pos.ticker_index = ticker_index;
    return pos;
  }

  bool is_empty_pos(const Position& pos) const {
    const auto& lp = pos.long_pos;
    const auto& sp = pos.short_pos;
    return lp.open_pending == 0 && lp.close_pending == 0 && lp.frozen == 0 &&
           lp.volume == 0 && sp.open_pending == 0 && sp.close_pending == 0 &&
           sp.frozen == 0 && sp.volume == 0;
  }

  Position* find(uint64_t ticker_index) {
    auto iter = pos_map_.find(ticker_index);
    if (iter == pos_map_.end()) return nullptr;
    return &iter->second;
  }

  const Position* find(uint64_t ticker_index) const {
    return const_cast<Portfolio*>(this)->find(ticker_index);
  }

 private:
  std::map<uint64_t, Position> pos_map_;
  double realized_pnl_ = 0;
};

}  // namespace ft

#endif  // FT_INCLUDE_PORTFOLIO_H_
