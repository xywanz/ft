// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_INCLUDE_CORE_RISKMANAGEMENTINTERFACE_H_
#define FT_INCLUDE_CORE_RISKMANAGEMENTINTERFACE_H_

#include "Core/Protocol.h"

namespace ft {

class RiskManagementInterface {
 public:
  virtual bool check_order_req(const OrderReq* req) { return false; }

  virtual void on_order_sent(uint64_t engine_order_id) {}

  /*
   * 订单成交时回调
   */
  virtual void on_order_traded(uint64_t engine_order_id, int this_traded,
                               double traded_price) {}

  virtual void on_order_completed(uint64_t engine_order_id, int error_code) {}
};

}  // namespace ft

#endif  // FT_INCLUDE_CORE_RISKMANAGEMENTINTERFACE_H_
