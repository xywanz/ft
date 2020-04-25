// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_INCLUDE_CONTEXT_H_
#define FT_INCLUDE_CONTEXT_H_

#include <algorithm>
#include <string>

#include "AlgoTrade/StrategyEngine.h"

namespace ft {

class AlgoTradeContext {
 public:
  explicit AlgoTradeContext(const std::string& ticker, StrategyEngine* se)
    : ticker_(ticker),
      se_(se) {
  }

  bool buy_open(int volume, OrderType type, double price) {
    return se_->buy_open(ticker_, volume, type, price);
  }

  bool sell_close(int volume, OrderType type, double price) {
    return se_->sell_close(ticker_, volume, type, price);
  }

  bool sell_open(int volume, OrderType type, double price) {
    return se_->sell_open(ticker_, volume, type, price);
  }

  bool buy_close(int volume, OrderType type, double price)  {
    return se_->buy_close(ticker_, volume, type, price);
  }

  int64_t buy(int64_t volume, double price, bool allow_part_traded = false) {
    if (volume <= 0 || price <= 1e-6)
      return false;

    const auto* pos = get_position();
    const auto& lp = pos->long_pos;
    const auto& sp = pos->short_pos;

    int64_t sell_pending = 0;
    sell_pending += sp.close_pending;
    sell_pending += lp.close_pending;

    if (sell_pending > 0)
      return -sell_pending;

    int64_t original_volume = volume;
    auto type = allow_part_traded ? OrderType::FAK : OrderType::FOK;
    if (sp.volume > 0) {
      int64_t to_close = std::min(sp.volume, volume);
      if (!buy_close(to_close, type, price))
        return 0;
      volume -= to_close;
    }

    if (volume > 0) {
      if (!buy_open(volume, type, price))
        return original_volume - volume;
    }

    return original_volume;
  }

  int64_t sell(int64_t volume, double price, bool allow_part_traded = false) {
    if (volume <= 0 || price <= 1e-6)
      return false;

    const auto* pos = get_position();
    const auto& lp = pos->long_pos;
    const auto& sp = pos->short_pos;

    int64_t buy_pending = 0;
    buy_pending += sp.close_pending;
    buy_pending += lp.open_pending;

    if (buy_pending > 0)
      return -buy_pending;

    int64_t original_volume = volume;
    auto type = allow_part_traded ? OrderType::FAK : OrderType::FOK;
    if (lp.volume > 0) {
      int64_t to_close = std::min(lp.volume, volume);
      if (!sell_close(to_close, type, price))
        return 0;
      volume -= to_close;
    }

    if (volume > 0) {
      if (!sell_open(volume, type, price))
        return original_volume - volume;
    }

    return original_volume;
  }

  bool cancel_order(const std::string& order_id) {
    return se_->cancel_order(order_id);
  }

  const Position* get_position() const {
    return se_->get_position(ticker_);
  }

  const MarketData* get_tick(std::size_t offset = 0) const {
    return se_->get_tick(ticker_, offset);
  }

  const std::string& this_ticker() const {
    return ticker_;
  }

 private:
  std::string ticker_;
  StrategyEngine* se_;
};

}  // namespace ft

#endif  // FT_INCLUDE_CONTEXT_H_
