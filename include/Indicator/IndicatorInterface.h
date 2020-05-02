// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_INCLUDE_INDICATOR_INDICATORINTERFACE_H_
#define FT_INCLUDE_INDICATOR_INDICATORINTERFACE_H_

#include "MarketData/Candlestick.h"

namespace ft {

class IndicatorInterface {
 public:
  virtual ~IndicatorInterface() {}

  virtual void on_init(const Candlestick* candlestick) {}

  virtual void on_bar() = 0;

  virtual void on_exit() {}
};

}  // namespace ft

#endif  // FT_INCLUDE_INDICATOR_INDICATORINTERFACE_H_
