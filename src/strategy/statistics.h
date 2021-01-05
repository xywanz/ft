// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_SRC_STRATEGY_STATISTICS_H_
#define FT_SRC_STRATEGY_STATISTICS_H_

#include "core/protocol.h"

namespace ft {

class Statistics {
 public:
  void Record(const OrderResponse& rsp);

 private:
};

}  // namespace ft

#endif  // FT_SRC_STRATEGY_STATISTICS_H_
