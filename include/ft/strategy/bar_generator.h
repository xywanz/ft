// Copyright [2020-2021] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_INCLUDE_FT_STRATEGY_BAR_GENERATOR_H_
#define FT_INCLUDE_FT_STRATEGY_BAR_GENERATOR_H_

#include <functional>
#include <unordered_map>

#include "ft/base/market_data.h"

namespace ft {

struct BarData {
  uint32_t ticker_id;
  uint32_t timestamp_us;

  double open;
  double high;
  double low;
  double close;
};

enum class BarPeriod { M1, M5, M15, H1 };

// 生成1分钟K线，并通过回调函数通知策略程序
//
// bar以跳空的方式生成，即假如某几分钟内没有tick，则不会生成新的bar
// 也不会通知策略程序
//
// TODO(Kevin)
// 1. 支持多个周期的K线生成
// 2. 生成真实的连续K线。假如某分钟内没有bar生成，则自动补全bar并通知策略
// 3. K线open价格采用上个K线的close价格
class BarGenerator {
 public:
  using Callback = std::function<void(const BarData&)>;

 public:
  BarGenerator();

  void AddPeriod(BarPeriod period, Callback&& cb);

  // 在策略程序的OnTick中调用该函数来更新K线
  void OnTick(const TickData& tick);

 private:
  std::unordered_map<uint32_t, BarData> bars_;  // ticker_id -> bar
  Callback m1_cb_;
};

}  // namespace ft

#endif  // FT_INCLUDE_FT_STRATEGY_BAR_GENERATOR_H_
