// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include <spdlog/spdlog.h>

#include "AlgoTrade/Strategy.h"

class GridStrategy : public ft::Strategy {
 public:
  void on_init(ft::AlgoTradeContext* ctx) override {
    spdlog::info("[GridStrategy::on_init]");

    subscribe({ticker_});
  }

  void on_tick(ft::AlgoTradeContext* ctx, const ft::TickData* tick) override {
    if (last_grid_price_ < 1e-6) last_grid_price_ = tick->last_price;

    const auto pos = ctx->get_position(ticker_);
    const auto& lp = pos.long_pos;
    const auto& sp = pos.short_pos;

    spdlog::info(
        "[GridStrategy::on_tick] last_price: {:.2f}, grid: {:.2f}, long: {}, "
        "short: {}, trades: {}, realized_pnl: {}, float_pnl: {}",
        tick->last_price, last_grid_price_, lp.volume, sp.volume, trade_counts_,
        ctx->get_realized_pnl(), ctx->get_float_pnl());

    if (tick->last_price - last_grid_price_ > grid_height_ - 1e-6) {
      ctx->sell_open(ticker_, trade_volume_each_, tick->bid[0]);
      spdlog::info(
          "[GRID] SELL VOLUME: {}, PRICE: {:.2f}, LAST:{:.2f}, PREV: {:.2f}",
          trade_volume_each_, tick->bid[0], tick->last_price, last_grid_price_);
      last_grid_price_ = tick->last_price;
      ++trade_counts_;
    } else if (tick->last_price - last_grid_price_ < -grid_height_ + 1e-6) {
      ctx->buy_open(ticker_, trade_volume_each_, tick->ask[0]);
      spdlog::info(
          "[GRID] BUY VOLUME: {}, PRICE: {:.2f}, LAST:{:.2f}, PREV: {:.2f}",
          trade_volume_each_, tick->ask[0], tick->last_price, last_grid_price_);
      last_grid_price_ = tick->last_price;
      ++trade_counts_;
    }
  }

  void on_order(ft::AlgoTradeContext* ctx, const ft::Order* order) {
    // if (order->status == ft::OrderStatus::CANCELED) {
    // }
  }

  void on_exit(ft::AlgoTradeContext* ctx) override {
    spdlog::info("[GridStrategy::on_exit]");
  }

 private:
  double last_grid_price_ = 0.0;
  double grid_height_ = 2;
  int trade_volume_each_ = 1;
  int trade_counts_ = 0;

  std::string ticker_ = "rb2009.SHFE";
};

EXPORT_STRATEGY(GridStrategy);
