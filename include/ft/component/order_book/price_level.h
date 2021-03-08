// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_INCLUDE_FT_COMPONENT_ORDER_BOOK_PRICE_LEVEL_H_
#define FT_INCLUDE_FT_COMPONENT_ORDER_BOOK_PRICE_LEVEL_H_

#include <list>

#include "ft/component/order_book/limit_order.h"

namespace ft::orderbook {

class PriceLevel {
 public:
  explicit PriceLevel(double price)
      : price_(price), decimal_price_(price_double_to_decimal(price)) {}

  void AddOrder(LimitOrder* order);
  void ModifyOrder(LimitOrder* order);
  void RemoveOrder(LimitOrder* order);

  double price() const { return price_; }
  uint64_t decimal_price() const { return decimal_price_; }
  int total_volume() const { return total_volume_; }

 private:
  double price_ = 0.0;
  uint64_t decimal_price_;
  int total_volume_ = 0;
  std::list<LimitOrder> order_list_;
};

}  // namespace ft::orderbook

#endif  // FT_INCLUDE_FT_COMPONENT_ORDER_BOOK_PRICE_LEVEL_H_
