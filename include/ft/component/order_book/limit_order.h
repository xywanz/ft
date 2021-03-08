// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_INCLUDE_FT_COMPONENT_ORDER_BOOK_LIMIT_ORDER_H_
#define FT_INCLUDE_FT_COMPONENT_ORDER_BOOK_LIMIT_ORDER_H_

#include <map>
#include <string>

namespace ft::orderbook {

inline uint64_t price_double_to_decimal(double p) {
  return (static_cast<uint64_t>(p * 100000000UL) + 50UL) / 10000UL;
}

inline double price_decimal_to_double(uint64_t p) { return static_cast<double>(p) / 10000; }

class PriceLevel;

class LimitOrder {
 public:
  LimitOrder(uint64_t _id, bool _is_buy, int _volume, double _price)
      : id_(_id),
        is_buy_(_is_buy),
        volume_(_volume),
        price_(_price),
        decimal_price_(price_double_to_decimal(_price)) {}

  void set_id(uint64_t i) { id_ = i; }
  uint64_t id() const { return id_; }

  void set_direction(bool _is_buy) { is_buy_ = _is_buy; }
  bool is_buy() const { return is_buy_; }

  void set_volume(int v) { volume_ = v; }
  int volume() const { return volume_; }

  void set_price(double p) {
    price_ = p;
    decimal_price_ = price_double_to_decimal(p);
  }
  double price() const { return price_; }
  uint64_t decimal_price() const { return decimal_price_; }

  void set_level(PriceLevel* l) { level_ = l; }
  const PriceLevel* level() { return level_; }

 private:
  uint64_t id_;
  bool is_buy_;
  int volume_;
  double price_;
  uint64_t decimal_price_;
  PriceLevel* level_;
};

}  // namespace ft::orderbook

#endif  // FT_INCLUDE_FT_COMPONENT_ORDER_BOOK_LIMIT_ORDER_H_
