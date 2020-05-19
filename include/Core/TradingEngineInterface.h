// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_INCLUDE_CORE_TRADINGENGINEINTERFACE_H_
#define FT_INCLUDE_CORE_TRADINGENGINEINTERFACE_H_

#include "Core/Account.h"
#include "Core/Contract.h"
#include "Core/Position.h"
#include "Core/TickData.h"
#include "Core/Trade.h"

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
  virtual void on_order_accepted(uint64_t order_id) {}

  /*
   * 订单被拒时回调
   */
  virtual void on_order_rejected(uint64_t order_id) {}

  /*
   * 订单成交时回调
   */
  virtual void on_order_traded(uint64_t order_id, int64_t this_traded,
                               double traded_price) {}

  /*
   * 撤单成功时回调
   */
  virtual void on_order_canceled(uint64_t order_id, int64_t canceled_volume) {}

  /*
   * 撤单被拒时回调
   */
  virtual void on_order_cancel_rejected(uint64_t order_id) {}
};

}  // namespace ft

#endif  // FT_INCLUDE_CORE_TRADINGENGINEINTERFACE_H_
