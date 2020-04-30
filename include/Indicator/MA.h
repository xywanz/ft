// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_INCLUDE_INDICATOR_MA_H_
#define FT_INCLUDE_INDICATOR_MA_H_

#include <string>
#include <vector>

#include "Indicator/IndicatorInterface.h"
#include "MarketData/TickDatabase.h"

namespace ft {

class MA : public IndicatorInterface {
 public:
  MA(uint64_t period_sec, uint64_t start_time_sec)
    : period_sec_(period_sec),
      start_time_sec_(start_time_sec) {}

  void on_init(const TickDatabase* db) override;

  void on_tick(const TickDatabase* db) override;

  void on_exit(const TickDatabase* db) override {}

 private:
  std::string ticker_;

  uint64_t period_sec_;
  uint64_t start_time_sec_;

  std::vector<double> result_;
};

}  // namespace ft



#endif  // FT_INCLUDE_INDICATOR_MA_H_
