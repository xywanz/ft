// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_INCLUDE_CEP_INTERFACE_OMS_INTERFACE_H_
#define FT_INCLUDE_CEP_INTERFACE_OMS_INTERFACE_H_

#include <string>

#include "cep/data/response.h"
#include "cep/data/tick_data.h"

namespace ft {

class OMSInterface {
 public:
  /*
   * 有新的tick数据到来时回调
   */
  virtual void on_tick(TickData* tick) {}

  /*
   * 订单被交易所接受时回调（如果只是被柜台而非交易所接受则不回调）
   */
  virtual void on_order_accepted(OrderAcceptance* rsp) {}

  /*
   * 订单被拒时回调
   */
  virtual void on_order_rejected(OrderRejection* rsp) {}

  /*
   * 订单成交时回调
   */
  virtual void on_order_traded(Trade* rsp) {}

  /*
   * 撤单成功时回调
   */
  virtual void on_order_canceled(OrderCancellation* rsp) {}

  /*
   * 撤单被拒时回调
   */
  virtual void on_order_cancel_rejected(OrderCancelRejection* rsp) {}
};

}  // namespace ft

#endif  // FT_INCLUDE_CEP_INTERFACE_OMS_INTERFACE_H_
