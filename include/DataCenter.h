// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_INCLUDE_DATACENTER_H_
#define FT_INCLUDE_DATACENTER_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "Base/DataStruct.h"
#include "MarketData/Candlestick.h"
#include "MarketData/TickDB.h"

namespace ft {

class DataCenter {
 public:
  void process_tick(const TickData* tick);

  const Candlestick* load_candlestick(const std::string& ticker);

  const TickDB* get_tickdb(const std::string& ticker) const {
    auto iter = tick_center_.find(ticker);
    if (iter == tick_center_.end()) return nullptr;
    return &iter->second;
  }

  const Candlestick* get_candlestick(const std::string& ticker) const {
    auto iter = candlestick_center_.find(ticker);
    if (iter == candlestick_center_.end()) return nullptr;
    return &iter->second;
  }

 private:
  std::map<std::string, TickDB> tick_center_;
  std::map<std::string, Candlestick> candlestick_center_;
};

}  // namespace ft

#endif  // FT_INCLUDE_DATACENTER_H_
