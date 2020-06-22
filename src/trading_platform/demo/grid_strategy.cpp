// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include <spdlog/spdlog.h>

#include "strategy/strategy.h"

class GridStrategy : public ft::Strategy {
 public:
  void on_init() override {
    spdlog::info("[GridStrategy::on_init]");

    subscribe({ticker_});
  }

  void on_tick(const ft::TickData& tick) override {
    if (last_grid_price_ < 1e-6) last_grid_price_ = tick.last_price;

    const auto pos = get_position(ticker_);
    const auto& lp = pos.long_pos;
    const auto& sp = pos.short_pos;

    // buy_open(ticker_, trade_volume_each_, tick.ask[0]);

    spdlog::info(
        "[GridStrategy::on_tick] last_price: {:.2f}, grid: {:.2f}, long: {}, "
        "short: {}, trades: {}",
        tick.last_price, last_grid_price_, lp.holdings, sp.holdings,
        trade_counts_);

    if (tick.last_price - last_grid_price_ > grid_height_ - 1e-6) {
      sell_open(ticker_, trade_volume_each_, tick.bid[0]);
      spdlog::info(
          "[GRID] SELL VOLUME: {}, PRICE: {:.2f}, LAST:{:.2f}, PREV: {:.2f}",
          trade_volume_each_, tick.bid[0], tick.last_price, last_grid_price_);
      last_grid_price_ = tick.last_price;
      ++trade_counts_;
    } else if (tick.last_price - last_grid_price_ < -grid_height_ + 1e-6) {
      buy_open(ticker_, trade_volume_each_, tick.ask[0]);
      spdlog::info(
          "[GRID] BUY VOLUME: {}, PRICE: {:.2f}, LAST:{:.2f}, PREV: {:.2f}",
          trade_volume_each_, tick.ask[0], tick.last_price, last_grid_price_);
      last_grid_price_ = tick.last_price;
      ++trade_counts_;
    }
  }

  void on_order_rsp(const ft::OrderResponse& order) override {
    spdlog::info("Order: {}  Traded/Total: {}/{}     Completed: {}",
                 order.order_id, order.traded_volume, order.original_volume,
                 order.completed);
  }

  void on_exit() override { spdlog::info("[GridStrategy::on_exit]"); }

 private:
  double last_grid_price_ = 0.0;
  double grid_height_ = 8;
  int trade_volume_each_ = 20;
  int trade_counts_ = 0;

  std::string ticker_ = "ni2008";
};

EXPORT_STRATEGY(GridStrategy);
