// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_INCLUDE_ALGOTRADE_STRATEGY_H_
#define FT_INCLUDE_ALGOTRADE_STRATEGY_H_

#include <memory>
#include <string>
#include <vector>

#include "AlgoTrade/Context.h"

namespace ft {

class Strategy {
 public:
  virtual ~Strategy() {}

  virtual bool on_init(AlgoTradeContext* ctx) {
    return true;
  }

  virtual void on_tick(AlgoTradeContext* ctx) {}

  virtual void on_order(AlgoTradeContext* ctx, const Order* order) {}

  virtual void on_trade(AlgoTradeContext* ctx, const Trade* trade) {}

  virtual void on_exit(AlgoTradeContext* ctx) {}

  bool is_mounted() const {
    return is_mounted_;
  }

 private:
  friend class StrategyEngine;
  void set_ctx(AlgoTradeContext* ctx) {
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
  std::unique_ptr<AlgoTradeContext> ctx_;
};

}  // namespace ft

#endif  // FT_INCLUDE_STRATEGY_H_
