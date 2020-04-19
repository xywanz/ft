// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_INCLUDE_CONTEXT_H_
#define FT_INCLUDE_CONTEXT_H_

#include <algorithm>
#include <string>

#include "TradingSystem.h"

namespace ft {

class QuantitativeTradingContext {
 public:
  explicit QuantitativeTradingContext(const std::string& ticker, TradingSystem* trader)
    : ticker_(ticker),
      ts_(trader) {
  }

  bool buy_open(int volume, OrderType type, double price) {
    return ts_->buy_open(ticker_, volume, type, price);
  }

  bool sell_close(int volume, OrderType type, double price) {
    return ts_->sell_close(ticker_, volume, type, price);
  }

  bool sell_open(int volume, OrderType type, double price) {
    return ts_->sell_open(ticker_, volume, type, price);
  }

  bool buy_close(int volume, OrderType type, double price)  {
    return ts_->buy_close(ticker_, volume, type, price);
  }

  int buy(int volume, double price, bool allow_part_traded = false) {
    if (volume <= 0 || price <= 1e-6)
      return false;

    auto* short_pos = get_position(Direction::SELL);
    auto* long_pos = get_position(Direction::BUY);

    int sell_pending = 0;
    sell_pending += short_pos ? short_pos->open_pending.load() : 0;
    sell_pending += long_pos ? long_pos->close_pending.load() : 0;

    if (sell_pending > 0)
      return -sell_pending;

    int original_volume = volume;
    auto type = allow_part_traded ? OrderType::FAK : OrderType::FOK;
    if (short_pos && short_pos->volume > 0) {
      int to_close = std::min(short_pos->volume, volume);
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

  int sell(int volume, double price, bool allow_part_traded = false) {
    if (volume <= 0 || price <= 1e-6)
      return false;

    auto* short_pos = get_position(Direction::SELL);
    auto* long_pos = get_position(Direction::BUY);

    int buy_pending = 0;
    buy_pending += short_pos ? short_pos->close_pending.load() : 0;
    buy_pending += long_pos ? long_pos->open_pending.load() : 0;

    if (buy_pending > 0)
      return -buy_pending;

    int original_volume = volume;
    auto type = allow_part_traded ? OrderType::FAK : OrderType::FOK;
    if (long_pos && long_pos->volume > 0) {
      int to_close = std::min(long_pos->volume, volume);
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
    return ts_->cancel_order(order_id);
  }

  const Position* get_position(Direction direction) const {
    return ts_->get_position(ticker_, direction);
  }

  const MarketData* get_tick(std::size_t offset = 0) const {
    return ts_->get_tick(ticker_, offset);
  }

  const std::string& this_ticker() const {
    return ticker_;
  }

 private:
  std::string ticker_;
  TradingSystem* ts_;
};

}  // namespace ft

#endif  // FT_INCLUDE_CONTEXT_H_
