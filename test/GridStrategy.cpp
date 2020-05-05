// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include <spdlog/spdlog.h>

#include "AlgoTrade/Strategy.h"
#include "MarketData/Candlestick.h"

class GridStrategy : public ft::Strategy {
 public:
  bool on_init(ft::AlgoTradeContext* ctx) override {
    spdlog::info("[GridStrategy::on_init]");

    const auto* tick = ctx->get_tick();
    if (!tick) return false;

    ctx->load_candlestick();

    const auto* pos = ctx->get_position();
    const auto& lp = pos->long_pos;
    const auto& sp = pos->short_pos;

    if (lp.volume > 0) {
      ctx->sell(lp.volume, tick->bid[0]);
      spdlog::info("[GridStrategy::on_init] Close all long pos");
    }

    if (sp.volume > 0) {
      ctx->buy(sp.volume, tick->ask[0]);
      spdlog::info("[GridStrategy::on_init] Close all short pos");
    }

    return true;
  }

  void on_tick(ft::AlgoTradeContext* ctx) override {
    const auto* tick = ctx->get_tick();

    const auto* bar = ctx->get_bar(cur_bar_);
    if (bar) {
      ++cur_bar_;
      spdlog::info(
          "[GridStrategy::on_tick] New 1m bar. open: {}, close: {}, "
          "high: {}, low: {}",
          bar->open, bar->close, bar->high, bar->low);
    }

    if (last_grid_price_ < 1e-6) last_grid_price_ = tick->last_price;

    const auto* pos = ctx->get_position();
    const auto& lp = pos->long_pos;
    const auto& sp = pos->short_pos;

    spdlog::info(
        "[GridStrategy::on_tick] last_price: {:.2f}, grid: {:.2f}, long: {}, "
        "short: {}, trades: {}, realized_pnl: {:.2f}, float_pnl: {:.2f}",
        ctx->get_tick()->last_price, last_grid_price_, lp.volume, sp.volume,
        trade_counts_, ctx->get_realized_pnl(), ctx->get_float_pnl());

    if (tick->last_price - last_grid_price_ > grid_height_ - 1e-6) {
      ctx->sell(trade_volume_each_, tick->bid[0]);
      spdlog::info(
          "[GRID] SELL VOLUME: {}, PRICE: {:.2f}, LAST:{:.2f}, PREV: {:.2f}",
          trade_volume_each_, tick->bid[0], tick->last_price, last_grid_price_);
      last_grid_price_ = tick->last_price;
      ++trade_counts_;
    } else if (tick->last_price - last_grid_price_ < -grid_height_ + 1e-6) {
      ctx->buy(trade_volume_each_, tick->ask[0]);
      spdlog::info(
          "[GRID] BUY VOLUME: {}, PRICE: {:.2f}, LAST:{:.2f}, PREV: {:.2f}",
          trade_volume_each_, tick->ask[0], tick->last_price, last_grid_price_);
      last_grid_price_ = tick->last_price;
      ++trade_counts_;
    }
  }

  void on_order(ft::AlgoTradeContext* ctx, const ft::Order* order) {
    const auto* tick = ctx->get_tick();
    if (!tick) return;

    // if (order->status == ft::OrderStatus::CANCELED) {
    // }
  }

  void on_exit(ft::AlgoTradeContext* ctx) override {
    spdlog::info("[GridStrategy::on_exit]");

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
  ft::Candlestick candle_chart_;
  std::size_t cur_bar_ = 1;
};

EXPORT_STRATEGY(GridStrategy);
