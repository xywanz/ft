// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_INCLUDE_CORE_TRADING_ENGINE_INTERFACE_H_
#define FT_INCLUDE_CORE_TRADING_ENGINE_INTERFACE_H_

#include "core/account.h"
#include "core/contract.h"
#include "core/position.h"
#include "core/tick_data.h"
#include "core/trade.h"

namespace ft {

class TradingEngineInterface {
 public:
  /*
   * 查询到合约时回调
   */
  virtual void on_query_contract(const Contract* contract) {}

  /*
   * 查询到账户信息时回调
   */
  virtual void on_query_account(const Account* account) {}

  /*
   * 查询到仓位信息时回调
   */
  virtual void on_query_position(const Position* position) {}

  virtual void on_query_trade(const Trade* trade) {}

  /*
   * 有新的tick数据到来时回调
   */
  virtual void on_tick(const TickData* tick) {}

  /*
   * 订单被交易所接受时回调（如果只是被柜台而非交易所接受则不回调）
   */
  virtual void on_order_accepted(uint64_t engine_order_id, uint64_t order_id) {}

  /*
   * 订单被拒时回调
   */
  virtual void on_order_rejected(uint64_t engine_order_id) {}

  /*
   * 订单成交时回调
   */
  virtual void on_order_traded(uint64_t engine_order_id, uint64_t order_id,
                               int this_traded, double traded_price) {}

  /*
   * 撤单成功时回调
   */
  virtual void on_order_canceled(uint64_t engine_order_id,
                                 int canceled_volume) {}

  /*
   * 撤单被拒时回调
   */
  virtual void on_order_cancel_rejected(uint64_t engine_order_id) {}
};

}  // namespace ft

#endif  // FT_INCLUDE_CORE_TRADING_ENGINE_INTERFACE_H_
