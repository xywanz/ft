// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_INCLUDE_INDICATOR_MA_H_
#define FT_INCLUDE_INDICATOR_MA_H_

#include <list>
#include <string>
#include <vector>

#include "Indicator/IndicatorInterface.h"

namespace ft {

class MA : public IndicatorInterface {
 public:
  explicit MA(uint64_t period)
    : period_(period) {}

  void on_init(const Candlestick* candlestick) override;

  void on_bar() override;

  void on_exit() override {}

 private:
  void update(const Bar* bar);

 private:
  const Candlestick* candlestick_ = nullptr;

  std::string ticker_;
  uint64_t period_;
  uint64_t bar_count_ = 0;
  double sum_ = 0;

  std::vector<double> ma_;
};

}  // namespace ft



#endif  // FT_INCLUDE_INDICATOR_MA_H_
