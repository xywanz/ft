// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_INCLUDE_INDICATOR_INDICATORINTERFACE_H_
#define FT_INCLUDE_INDICATOR_INDICATORINTERFACE_H_

#include "Base/DataStruct.h"
#include "MarketData/TickDatabase.h"

namespace ft {

class IndicatorInterface {
 public:
  virtual ~IndicatorInterface() {}

  virtual void on_init(const TickDatabase* db) {}

  virtual void on_tick(const TickDatabase* db) = 0;

  virtual void on_exit(const TickDatabase* db) {}
};

}  // namespace ft

#endif  // FT_INCLUDE_INDICATOR_INDICATORINTERFACE_H_
