// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_SRC_STRATEGY_ORDER_MANAGER_H_
#define FT_SRC_STRATEGY_ORDER_MANAGER_H_

#include <list>
#include <map>

#include "core/protocol.h"

namespace ft {

class OrderManager {
 public:
  void on_order(const OrderResponse* rsp);

 private:
};

}  // namespace ft

#endif  // FT_SRC_STRATEGY_ORDER_MANAGER_H_
