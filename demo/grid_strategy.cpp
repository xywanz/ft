// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "ft/strategy/strategy.h"
#include "spdlog/spdlog.h"

class GridStrategy : public ft::Strategy {
 public:
  void OnInit() override {
    spdlog::info("[GridStrategy::on_init]");

    Subscribe({ticker_});
  }

  void OnTick(const ft::TickData& tick) override {
    if (tick.last_price < 1e-6) {
      return;
    }

    if (last_grid_price_ < 1e-6) {
      last_grid_price_ = tick.last_price;
    }

    const auto pos = GetPosition(ticker_);
    const auto& lp = pos.long_pos;
    const auto& sp = pos.short_pos;

    spdlog::info(
        "GridStrategy::on_tick {} last_price:{:.2f}, grid:{:.2f}, long:{}, short:{}, trades:{}",
        tick.exchange_timestamp_us, tick.last_price, last_grid_price_, lp.holdings, sp.holdings,
        trade_counts_);

    if (tick.last_price - last_grid_price_ > grid_height_) {
      int vol_to_close = std::min(lp.holdings, trade_volume_each_);
      int vol_to_open = std::max(0, trade_volume_each_ - vol_to_close);
      if (vol_to_close > 0) {
        SellClose(ticker_, vol_to_close, tick.bid[0]);
      }
      if (vol_to_open > 0) {
        SellOpen(ticker_, vol_to_open, tick.bid[0]);
      }
      spdlog::info("[GRID] SELL VOLUME: {}, PRICE: {:.2f}, LAST:{:.2f}, PREV: {:.2f}",
                   trade_volume_each_, tick.bid[0], tick.last_price, last_grid_price_);
      last_grid_price_ = tick.last_price;
      ++trade_counts_;
    } else if (tick.last_price - last_grid_price_ < -grid_height_) {
      int vol_to_close = std::min(sp.holdings, trade_volume_each_);
      int vol_to_open = std::max(0, trade_volume_each_ - vol_to_close);
      if (vol_to_close > 0) {
        BuyClose(ticker_, vol_to_close, tick.ask[0]);
      }
      if (vol_to_open > 0) {
        BuyOpen(ticker_, vol_to_open, tick.ask[0]);
      }
      spdlog::info("[GRID] BUY VOLUME: {}, PRICE: {:.2f}, LAST:{:.2f}, PREV: {:.2f}",
                   trade_volume_each_, tick.ask[0], tick.last_price, last_grid_price_);
      last_grid_price_ = tick.last_price;
      ++trade_counts_;
    }
  }

  void OnOrder(const ft::OrderResponse& order) override {
    spdlog::info("Order: {}  Traded/Total: {}/{}     Completed: {}", order.order_id,
                 order.traded_volume, order.original_volume, order.completed);
  }

  void OnExit() override { spdlog::info("[GridStrategy::OnExit]"); }

 private:
  double last_grid_price_ = 0.0;
  double grid_height_ = 8;
  int trade_volume_each_ = 20;
  int trade_counts_ = 0;

  std::string ticker_ = "UR101";
};

EXPORT_STRATEGY(GridStrategy);
