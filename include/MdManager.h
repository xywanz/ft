// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_INCLUDE_MDMANAGER_H_
#define FT_INCLUDE_MDMANAGER_H_

#include <string>
#include <vector>

#include "Common.h"
#include "MarketData.h"

namespace ft {

enum class Period {
  M1,
};

struct KLine {
  double open = 0;
  double close = 0;
  double high = 0;
  double low = 0;
};

using KChart = std::vector<KLine>;

class MdManager {
 public:
  explicit MdManager(const std::string& ticker = "");

  const KLine* get_kline(Period period, uint64_t offset) const {
    switch (period) {
    case Period::M1:
      if (offset >= kchart_m1_.size())
        return nullptr;
      return &kchart_m1_.back() - offset;
    default:
      return nullptr;
    }
  }

  const KChart* get_kchart(Period period) const {
    switch (period) {
    case Period::M1:
      return &kchart_m1_;
    default:
      return nullptr;
    }
  }

  void on_tick(const MarketData* data);

 private:
  std::string ticker_;

  std::vector<MarketData> tick_data_;

  KChart kchart_m1_;
  KLine last_kline_m1_;
  uint64_t last_mininute_ = 0;

  uint64_t volume_;
  uint64_t open_interest_;
  uint64_t turnover_;
};

}  // namespace ft

#endif  // FT_INCLUDE_MDMANAGER_H_
