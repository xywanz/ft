// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_INCLUDE_ENGINE_H_
#define FT_INCLUDE_ENGINE_H_

#include <string>

#include "Account.h"
#include "Contract.h"
#include "Common.h"
#include "LoginParams.h"
#include "MarketData.h"
#include "Order.h"
#include "Position.h"
#include "Trade.h"

namespace ft {

class Engine {
 public:
  virtual ~Engine() {}

  virtual void on_position(const Position* position) {}

  virtual void on_account(const Account* account) {}

  virtual void on_contract(const Contract* contract) {}

  virtual void on_order(const Order* order) {}

  virtual void on_trade(const Trade* trade) {}

  virtual void on_tick(const MarketData* data) {}
};

}  // namespace ft

#endif  // FT_INCLUDE_ENGINE_H_
