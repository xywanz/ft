// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_INCLUDE_STRATEGY_H_
#define FT_INCLUDE_STRATEGY_H_

#include <memory>
#include <string>
#include <vector>

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


class Strategy {
 public:
  virtual ~Strategy() {}

  virtual void on_init(QuantitativeTradingContext* ctx) {}

  virtual void on_tick(QuantitativeTradingContext* ctx) {}

  virtual void on_exit(QuantitativeTradingContext* ctx) {}

  bool is_mounted() const {
    return is_mounted_;
  }

 private:
  friend class TradingSystem;
  void set_ctx(QuantitativeTradingContext* ctx) {
    std::unique_lock<std::mutex> lock(mutex_);
    if (is_mounted_)
      return;
    ctx_.reset(ctx);
    is_mounted_ = ctx ? true : false;
  }

  auto get_ctx() const {
    return ctx_.get();
  }

 private:
  bool is_mounted_ = false;
  std::mutex mutex_;
  std::unique_ptr<QuantitativeTradingContext> ctx_;
};

}  // namespace ft

#endif  // FT_INCLUDE_STRATEGY_H_
