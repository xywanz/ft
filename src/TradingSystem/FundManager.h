// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_SRC_TRADINGSYSTEM_FUNDMANAGER_H_
#define FT_SRC_TRADINGSYSTEM_FUNDMANAGER_H_

#include "Core/Account.h"
#include "TradingSystem/Order.h"

namespace ft {

class FundManager {
 public:
  void init(Account* account);
  void on_new_order(const Order& order);
  void on_order_abort(const Order& order, int incompleted_volume);
  void on_order_traded(const Order& order, int traded, double traded_price);

 private:
  Account* account_{nullptr};
};

}  // namespace ft

#endif  // FT_SRC_TRADINGSYSTEM_FUNDMANAGER_H_
