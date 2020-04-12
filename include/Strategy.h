// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_INCLUDE_STRATEGY_H_
#define FT_INCLUDE_STRATEGY_H_

#include <string>
#include <vector>

#include "Trader.h"

namespace ft {

class QuantitativTradingContext {
 public:
  explicit QuantitativTradingContext(Trader* trader)
    : trader_(trader) {
  }

  bool subscribe(const std::vector<std::string>& tickers) {
    auto n = trader_->subscribe(tickers);
    if (n != tickers.size())
      return false;

    return true;
  }

  bool buy_open(const std::string& ticker, int volume,
                OrderType type, double price = 0) {
    return trader_->buy_open(ticker, volume, type, price);
  }

  bool sell_close(const std::string& ticker, int volume,
                  OrderType type, double price = 0) {
    return trader_->sell_close(ticker, volume, type, price);
  }

  bool sell_open(const std::string& ticker, int volume,
                 OrderType type, double price = 0) {
    return trader_->sell_open(ticker, volume, type, price);
  }

  bool buy_close(const std::string& ticker, int volume,
                 OrderType type, double price = 0)  {
    return trader_->buy_close(ticker, volume, type, price);
  }

  bool cancel_order(const std::string& order_id) {
    return trader_->cancel_order(order_id);
  }

  void get_orders(std::vector<const Order*>* out) {
    return trader_->get_orders(out);
  }

  void get_orders(const std::string&ticker,
                  std::vector<const Order*>* out) {
    return trader_->get_orders(ticker, out);
  }

 private:
  Trader* trader_;
};


class Strategy {
 public:
  virtual void on_init(QuantitativTradingContext* ctx) {
  }

  virtual void on_tick(QuantitativTradingContext* ctx) {
  }

  virtual void on_exit(QuantitativTradingContext* ctx) {
  }

 private:
};

}  // namespace ft

#endif  // FT_INCLUDE_STRATEGY_H_
