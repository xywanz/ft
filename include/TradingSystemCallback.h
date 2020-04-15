// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_INCLUDE_TRADINGSYSTEMCALLBACK_H_
#define FT_INCLUDE_TRADINGSYSTEMCALLBACK_H_

#include <string>

#include "Account.h"
#include "Contract.h"
#include "LoginParams.h"
#include "MarketData.h"
#include "Position.h"
#include "Order.h"
#include "Trade.h"

namespace ft {

class TradingSystemCallback {
 public:
  virtual void on_contract(const Contract* contract) {}

  /*
   * 接收查询到的汇总仓位数据
   * 当gateway触发了一次仓位查询时，需要把仓位缓存并根据{ticker-direction}
   * 进行汇总，每个{ticker-direction}对应一个Position对象，本次查询完成后，
   * 对每个汇总的Position对象回调Trader::on_position
   */
  virtual void on_position(const Position* position) {}

  /*
   * 接收查询到的账户信息
   */
  virtual void on_account(const Account* account) {}

  /*
   * 接受订单信息
   * 当订单状态发生改变时触发
   */
  virtual void on_order(const Order* order) {}

  /*
   * 接受成交信息
   * 每笔成交都会回调
   */
  virtual void on_trade(const Trade* trade) {}

  /*
   * 接受行情数据
   * 每一个tick都会回调
   */
  virtual void on_market_data(const MarketData* data) {}
};

}  // namespace ft

#endif  // FT_INCLUDE_TRADINGSYSTEMCALLBACK_H_
