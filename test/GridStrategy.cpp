// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include <spdlog/spdlog.h>

#include "AlgoTrade/Strategy.h"

class GridStrategy : public ft::Strategy {
 public:
  bool on_init(ft::AlgoTradeContext* ctx) override {
    spdlog::info("[MyStrategy::on_init]");

    const auto* tick = ctx->get_tick();
    if (!tick)
      return false;

    const auto* pos = ctx->get_position();
    const auto& lp = pos->long_pos;
    const auto& sp = pos->short_pos;

    if (lp.volume > 0) {
      ctx->sell(lp.volume, tick->bid[0]);
      spdlog::info("Close all long pos");
    }

    if (sp.volume > 0) {
      ctx->buy(sp.volume, tick->ask[0]);
      spdlog::info("Close all short pos");
    }

    return true;
  }

  void on_tick(ft::AlgoTradeContext* ctx) override {
    const auto* tick = ctx->get_tick();

    if (last_grid_price_ < 1e-6)
      last_grid_price_ = tick->last_price;

    const auto* pos = ctx->get_position();
    const auto& lp = pos->long_pos;
    const auto& sp = pos->short_pos;

    spdlog::info(
      "[MyStrategy::on_tick] last_price: {:.2f}, grid: {:.2f}, long: {}, short: {}, trades: {}",
      ctx->get_tick()->last_price, last_grid_price_, lp.volume, sp.volume, trade_counts_);

    if (tick->last_price - last_grid_price_ > grid_height_ - 1e-6) {
      ctx->sell(trade_volume_each_, tick->bid[0]);
      spdlog::info("[GRID] SELL VOLUME: {}, PRICE: {:.2f}, LAST:{:.2f}, PREV: {:.2f}",
                   trade_volume_each_, tick->bid[0], tick->last_price, last_grid_price_);
      last_grid_price_ = tick->last_price;
      ++trade_counts_;
    } else if (tick->last_price - last_grid_price_ < -grid_height_ + 1e-6) {
      ctx->buy(trade_volume_each_, tick->ask[0]);
      spdlog::info("[GRID] BUY VOLUME: {}, PRICE: {:.2f}, LAST:{:.2f}, PREV: {:.2f}",
                   trade_volume_each_, tick->ask[0], tick->last_price, last_grid_price_);
      last_grid_price_ = tick->last_price;
      ++trade_counts_;
    }
  }

  void on_order(ft::AlgoTradeContext* ctx, const ft::Order* order) {
    const auto* tick = ctx->get_tick();
    if (!tick)
      return;

    // if (order->status == ft::OrderStatus::CANCELED) {
    // }
  }

  void on_exit(ft::AlgoTradeContext* ctx) override {
    spdlog::info("[MyStrategy::on_exit]");

    const auto* pos = ctx->get_position();
    const auto& lp = pos->long_pos;
    const auto& sp = pos->short_pos;

    if (lp.volume > 0) {
      ctx->sell(lp.volume, 3300);
      spdlog::info("Close all long pos");
    }

    if (sp.volume > 0) {
      ctx->buy(sp.volume, 3600);
      spdlog::info("Close all short pos");
    }
  }

 private:
  double last_grid_price_ = 0.0;
  double grid_height_ = 10.0;
  int trade_volume_each_ = 100;
  int trade_counts_ = 0;
};

extern "C" void* create_strategy() {
  return new GridStrategy;
}
