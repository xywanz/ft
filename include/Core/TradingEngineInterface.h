// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_INCLUDE_TRADINGENGINEINTERFACE_H_
#define FT_INCLUDE_TRADINGENGINEINTERFACE_H_

#include "Core/Account.h"
#include "Core/Contract.h"
#include "Core/Position.h"
#include "Core/TickData.h"

namespace ft {

class TradingEngineInterface {
 public:
  virtual void on_query_contract(const Contract* contract) {}

  virtual void on_query_account(const Account* account) {}

  virtual void on_query_position(const Position* position) {}

  virtual void on_tick(const TickData* tick) {}

  virtual void on_order_accepted(uint64_t order_id) {}

  virtual void on_order_rejected(uint64_t order_id) {}

  virtual void on_order_traded(uint64_t order_id, int64_t this_traded,
                               double traded_price) {}

  virtual void on_order_canceled(uint64_t order_id, int64_t canceled_volume) {}

  virtual void on_order_cancel_rejected(uint64_t order_id) {}
};

}  // namespace ft

#endif  // FT_INCLUDE_TRADINGENGINEINTERFACE_H_
