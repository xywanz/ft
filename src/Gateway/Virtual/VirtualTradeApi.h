// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_SRC_GATEWAY_VIRTUAL_VIRTUALTRADEAPI_H_
#define FT_SRC_GATEWAY_VIRTUAL_VIRTUALTRADEAPI_H_

#include "Core/Constants.h"

namespace ft {

struct VirtualOrderReq {
  uint64_t order_id;
  uint32_t ticker_index;
  uint32_t type;
  uint32_t direction;
  uint32_t offset;
  int volume = 0;
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
