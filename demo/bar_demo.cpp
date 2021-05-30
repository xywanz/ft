// Copyright [2020-2021] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include <memory>

#include "ft/strategy/bar_generator.h"
#include "ft/strategy/strategy.h"
#include "spdlog/spdlog.h"

class BarDemo : public ft::Strategy {
 public:
  void OnInit() override {
    bar_generator_.AddPeriod(ft::BarPeriod::M1, [this](const ft::BarData& bar) { OnBar(bar); });

    Subscribe({"rb2105"});
  }

  void OnTick(const ft::TickData& tick) { bar_generator_.OnTick(tick); }

  void OnBar(const ft::BarData& bar) {
    spdlog::info("on_bar: open:{}, high:{}, low:{}, close:{}, ticker:{}, timestamp:{}", bar.open,
                 bar.high, bar.low, bar.close, bar.ticker_id, bar.timestamp_us);
  }

 private:
  ft::BarGenerator bar_generator_;
};

EXPORT_STRATEGY(BarDemo);
