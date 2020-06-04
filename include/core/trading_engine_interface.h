// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_INCLUDE_CORE_TRADING_ENGINE_INTERFACE_H_
#define FT_INCLUDE_CORE_TRADING_ENGINE_INTERFACE_H_

#include "core/account.h"
#include "core/contract.h"
#include "core/position.h"
#include "core/tick_data.h"

namespace ft {

struct OrderAcceptedRsp {
  uint64_t engine_order_id;
  uint64_t order_id;
};

struct OrderTradedRsp {
  uint64_t engine_order_id;
  uint64_t order_id;
  uint32_t ticker_index;
  uint32_t direction;
  uint32_t offset;
  uint32_t trade_type;
  int volume;
  double price;
  double amount;
};

struct OrderRejectedRsp {
  uint64_t engine_order_id;
};

struct OrderCanceledRsp {
  uint64_t engine_order_id;
  int canceled_volume;
};

struct OrderCancelRejectedRsp {
  uint64_t engine_order_id;
};

class TradingEngineInterface {
 public:
  /*
   * 查询到合约时回调
   */
  virtual void on_query_contract(Contract* contract) {}

  /*
   * 查询到账户信息时回调
   */
  virtual void on_query_account(Account* account) {}

  /*
   * 查询到仓位信息时回调
   */
  virtual void on_query_position(Position* position) {}

  virtual void on_query_trade(OrderTradedRsp* trade) {}

  /*
   * 有新的tick数据到来时回调
   */
  virtual void on_tick(TickData* tick) {}

  /*
   * 订单被交易所接受时回调（如果只是被柜台而非交易所接受则不回调）
   */
  virtual void on_order_accepted(OrderAcceptedRsp* rsp) {}

  /*
   * 订单被拒时回调
   */
  virtual void on_order_rejected(OrderRejectedRsp* rsp) {}

  /*
   * 订单成交时回调
   */
  virtual void on_order_traded(OrderTradedRsp* rsp) {}

  /*
   * 撤单成功时回调
   */
  virtual void on_order_canceled(OrderCanceledRsp* rsp) {}

  /*
   * 撤单被拒时回调
   */
  virtual void on_order_cancel_rejected(OrderCancelRejectedRsp* rsp) {}
};

}  // namespace ft

#endif  // FT_INCLUDE_CORE_TRADING_ENGINE_INTERFACE_H_
