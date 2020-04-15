// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_INCLUDE_STRATEGY_H_
#define FT_INCLUDE_STRATEGY_H_

#include <string>
#include <vector>

#include "TradingSystem.h"

namespace ft {

class QuantitativTradingContext {
 public:
  explicit QuantitativTradingContext(const std::string& ticker, TradingSystem* trader)
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

  virtual void on_init(QuantitativTradingContext* ctx) {}

  virtual void on_tick(QuantitativTradingContext* ctx) {}

  virtual void on_exit(QuantitativTradingContext* ctx) {}

 private:
  friend class TradingSystem;
  void set_ctx(QuantitativTradingContext* ctx) {
    ctx_ = ctx;
  }

  auto get_ctx() const {
    return ctx_;
  }

 private:
  QuantitativTradingContext* ctx_;
};

}  // namespace ft

#endif  // FT_INCLUDE_STRATEGY_H_
