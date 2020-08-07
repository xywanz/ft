// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_INCLUDE_CEP_DATA_CONTRACT_H_
#define FT_INCLUDE_CEP_DATA_CONTRACT_H_

#include <string>

namespace ft {

enum class ProductType {
  FUTURES = 0,
  OPTIONS,
  STOCK,
  FUND,
  UNKNOWN,
};

enum SpecTickerIndex : uint32_t {
  NONE_TICKER = 0,
  ALL_TICKERS = UINT32_MAX,
};

struct Contract {
  std::string ticker;
  std::string exchange;
  std::string name;
  ProductType product_type;
  int size;
  double price_tick;

  double long_margin_rate;
  double short_margin_rate;

  int max_market_order_volume;
  int min_market_order_volume;
  int max_limit_order_volume;
  int min_limit_order_volume;

  int delivery_year;
  int delivery_month;

  uint32_t tid;  // local index
};

inline std::string to_string(ProductType product) {
  if (product == ProductType::FUTURES) return "Futures";
  if (product == ProductType::OPTIONS) return "Options";
  if (product == ProductType::STOCK) return "Stock";
  if (product == ProductType::FUND) return "Fund";

  return "Unknown";
}

inline ProductType string2product(const std::string& product) {
  if (product == "Futures") return ProductType::FUTURES;
  if (product == "Options") return ProductType::OPTIONS;
  if (product == "Stock") return ProductType::STOCK;
  if (product == "Fund") return ProductType::FUND;

  return ProductType::UNKNOWN;
}

}  // namespace ft

#endif  // FT_INCLUDE_CEP_DATA_CONTRACT_H_
