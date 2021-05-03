// Copyright [2020-2021] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_SRC_TRADER_TRADER_SERVER_H_
#define FT_SRC_TRADER_TRADER_SERVER_H_

#include <string>

#include "ft/base/trade_msg.h"

namespace ft {

class TraderServer {
 public:
  void Start();

 private:
  int epfd_;
};

}  // namespace ft

#endif  // FT_SRC_TRADER_TRADER_SERVER_H_
