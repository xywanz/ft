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

  void update_pending(const std::string& ticker, Direction direction,
                      Offset offset, int changed);

  void update_traded(const std::string& ticker, Direction direction,
                     Offset offset, int64_t traded, double traded_price);

  void update_float_pnl(const std::string& ticker, double last_price);

  const Position* get_position_unsafe(const std::string& ticker) const {
    static const Position empty_pos;

    const auto* pos = find(ticker);
    return pos ? pos : &empty_pos;
  }

  void get_pos_ticker_list_unsafe(std::vector<const std::string*>* out) const {
    for (const auto& [ticker, pos] : pos_map_) {
      if (!is_empty_pos(pos)) out->emplace_back(&ticker);
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
    if (iter == pos_map_.end()) return nullptr;
    return &iter->second;
  }

  const Position* find(const std::string& ticker) const {
    return const_cast<Portfolio*>(this)->find(ticker);
  }

 private:
  std::map<std::string, Position> pos_map_;
  double realized_pnl_ = 0;
};

}  // namespace ft

#endif  // FT_INCLUDE_PORTFOLIO_H_
