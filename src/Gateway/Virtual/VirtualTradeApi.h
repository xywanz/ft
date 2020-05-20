// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_SRC_GATEWAY_VIRTUAL_VIRTUALTRADEAPI_H_
#define FT_SRC_GATEWAY_VIRTUAL_VIRTUALTRADEAPI_H_

#include "Core/Constants.h"

namespace ft {

struct VirtualOrderReq {
  uint64_t order_id;
  uint64_t ticker_index;
  uint64_t type;
  uint64_t direction;
  uint64_t offset;
  int64_t volume = 0;
  double price = 0;
};

class VirtualGateway;

class VirtualTradeApi {
 public:
  bool insert_order(const VirtualOrderReq* req);

 private:
  VirtualGateway* gateway_;
};

}  // namespace ft

#endif  // FT_SRC_GATEWAY_VIRTUAL_VIRTUALTRADEAPI_H_
