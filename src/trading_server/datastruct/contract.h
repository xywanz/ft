// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_SRC_TRADING_SERVER_DATASTRUCT_CONTRACT_H_
#define FT_SRC_TRADING_SERVER_DATASTRUCT_CONTRACT_H_

#include <string>

namespace ft {

enum class ProductType {
  kFutures = 0,
  kOptions,
  kStock,
  kFund,
  kUnknown,
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

  uint32_t ticker_id;  // local index
};

inline std::string ToString(ProductType product) {
  if (product == ProductType::kFutures) return "Futures";
  if (product == ProductType::kOptions) return "Options";
  if (product == ProductType::kStock) return "Stock";
  if (product == ProductType::kFund) return "Fund";

  return "Unknown";
}

inline ProductType string2product(const std::string& product) {
  if (product == "Futures") return ProductType::kFutures;
  if (product == "Options") return ProductType::kOptions;
  if (product == "Stock") return ProductType::kStock;
  if (product == "Fund") return ProductType::kFund;

  return ProductType::kUnknown;
}

}  // namespace ft

#endif  // FT_SRC_TRADING_SERVER_DATASTRUCT_CONTRACT_H_
