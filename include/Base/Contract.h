// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_INCLUDE_BASE_CONTRACT_H_
#define FT_INCLUDE_BASE_CONTRACT_H_

#include <cppex/string.h>
#include <fmt/format.h>

#include <cassert>
#include <fstream>
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace ft {

enum class ProductType { FUTURES = 0, OPTIONS };

struct Contract {
  std::string symbol;
  std::string exchange;
  std::string ticker;
  std::string name;
  ProductType product_type;
  int64_t size;
  double price_tick;

  int64_t max_market_order_volume;
  int64_t min_market_order_volume;
  int64_t max_limit_order_volume;
  int64_t min_limit_order_volume;

  int delivery_year;
  int delivery_month;

  uint64_t index;  // local index
};

inline const std::string& to_string(ProductType product) {
  static const std::map<ProductType, std::string> product_str = {
      {ProductType::FUTURES, "Futures"}, {ProductType::OPTIONS, "Options"}};

  return product_str.find(product)->second;
}

inline ProductType string2product(const std::string& product) {
  static const std::map<std::string, ProductType> product_map = {
      {"Futures", ProductType::FUTURES}, {"Options", ProductType::OPTIONS}};

  return product_map.find(product)->second;
}

}  // namespace ft

#endif  // FT_INCLUDE_BASE_CONTRACT_H_
