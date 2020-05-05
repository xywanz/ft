// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_INCLUDE_ALGOTRADE_CONTEXT_H_
#define FT_INCLUDE_ALGOTRADE_CONTEXT_H_

#include <algorithm>
#include <string>

#include "AlgoTrade/StrategyEngine.h"

namespace ft {

class AlgoTradeContext {
 public:
  explicit AlgoTradeContext(const std::string& ticker, StrategyEngine* engine,
                            DataCenter* data_center, const TradingPanel* panel)
      : ticker_(ticker),
        panel_(panel),
        engine_(engine),
        data_center_(data_center) {
    db_ = data_center_->get_tickdb(ticker);
  }

  std::string buy_open(int volume, double price,
                       OrderType type = OrderType::FAK) {
    return engine_->buy_open(ticker_, volume, type, price);
  }

  std::string sell_close(int volume, double price,
                         OrderType type = OrderType::FAK) {
    return engine_->sell_close(ticker_, volume, type, price);
  }

  std::string sell_open(int volume, double price,
                        OrderType type = OrderType::FAK) {
    return engine_->sell_open(ticker_, volume, type, price);
  }

  std::string buy_close(int volume, double price,
                        OrderType type = OrderType::FAK) {
    return engine_->buy_close(ticker_, volume, type, price);
  }

  int64_t buy(int64_t volume, double price, OrderType type = OrderType::FAK) {
    if (volume <= 0 || price <= 1e-6) return false;

    const auto* pos = get_position();
    const auto& lp = pos->long_pos;
    const auto& sp = pos->short_pos;

    int64_t sell_pending = 0;
    sell_pending += sp.close_pending;
    sell_pending += lp.close_pending;

    if (sell_pending > 0) return -sell_pending;

    int64_t original_volume = volume;
    if (sp.volume > 0) {
      int64_t to_close = std::min(sp.volume, volume);
      if (buy_close(to_close, price, type) == "") return 0;
      volume -= to_close;
    }

    if (volume > 0) {
      if (buy_open(volume, price, type) == "") return original_volume - volume;
    }

    return original_volume;
  }

  int64_t sell(int64_t volume, double price, OrderType type = OrderType::FAK) {
    if (volume <= 0 || price <= 1e-6) return false;

    const auto* pos = get_position();
    const auto& lp = pos->long_pos;
    const auto& sp = pos->short_pos;

    int64_t buy_pending = 0;
    buy_pending += sp.close_pending;
    buy_pending += lp.open_pending;

    if (buy_pending > 0) return -buy_pending;

    int64_t original_volume = volume;
    if (lp.volume > 0) {
      int64_t to_close = std::min(lp.volume, volume);
      if (sell_close(to_close, price, type) == "") return 0;
      volume -= to_close;
    }

    if (volume > 0) {
      if (sell_open(volume, price, type) == "") return original_volume - volume;
    }

    return original_volume;
  }

  bool cancel_order(const std::string& order_id) {
    return engine_->cancel_order(order_id);
  }

  void cancel_all() { engine_->cancel_all(ticker_); }

  void unmount() {
    engine_->unmount_strategy(stra)
  }

  double get_realized_pnl() const { return panel_->get_realized_pnl(); }

  double get_float_pnl() const { return panel_->get_float_pnl(); }

  void load_candlestick() {
    candlestick_ = data_center_->load_candlestick(ticker_);
  }

  const Bar* get_bar(std::size_t offset) const {
    if (!candlestick_) {
      spdlog::error("[AlgoTradeContext::get_bar] Candle chart not loaded");
      return nullptr;
    }
    return candlestick_->get_bar(offset);
  }

  const Position* get_position() const { return panel_->get_position(ticker_); }

  const TickData* get_tick(std::size_t offset = 0) const {
    return db_->get_tick(offset);
  }

  const std::string& this_ticker() const { return ticker_; }

 private:
  std::string ticker_;
  StrategyEngine* engine_ = nullptr;
  DataCenter* data_center_ = nullptr;
  const TradingPanel* panel_ = nullptr;
  const TickDB* db_ = nullptr;
  const Candlestick* candlestick_ = nullptr;
};

}  // namespace ft

#endif  // FT_INCLUDE_ALGOTRADE_CONTEXT_H_
