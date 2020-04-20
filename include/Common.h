// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_INCLUDE_COMMON_H_
#define FT_INCLUDE_COMMON_H_

#include <atomic>
#include <map>
#include <memory>
#include <string>

#include <fmt/format.h>

namespace ft {

inline const std::string kSHFE = "SHFE";
inline const std::string kINE = "INE";
inline const std::string kCFFEX = "CFFEX";
inline const std::string kCZCE = "CZCE";
inline const std::string kDCE = "DCE";

enum class Direction {
  BUY = 0,
  SELL,
};

enum class Offset {
  OPEN = 0,
  CLOSE,
  CLOSE_TODAY,
  CLOSE_YESTERDAY,
  FORCE_CLOSE,
  FORCE_OFF,
  LOCAL_FORCE_CLOSE,
};

enum class CombHedgeFlag {
  SPECULATION = 0,
  ARBITRAGE,
  HEDGE,
};

enum class OrderType {
  LIMIT = 0,      // 市价
  MARKET,         //
  FAK,            //
  FOK,            //
  BEST,           // 最优价
};

enum class ProductType {
  FUTURES = 0,
  OPTIONS
};

enum class OrderStatus {
  SUBMITTING = 0,
  REJECTED,
  NO_TRADED,
  PART_TRADED,
  ALL_TRADED,
  CANCELED,
  CANCEL_REJECTED
};

enum class FrontType {
  CTP
};

// such as rb2009.SHFE
inline std::string to_ticker(const std::string& symbol,
                          const std::string& exchange) {
  return fmt::format("{}.{}", symbol, exchange);
}

inline void ticker_split(const std::string& ticker,
                        std::string* symbol,
                        std::string* exchange) {
  auto pos = ticker.find_first_of('.');
  *symbol = ticker.substr(0, pos);
  if (pos != std::string::npos && pos + 1 < ticker.size())
    *exchange = ticker.substr(pos + 1);
}

inline Direction opp_direction(Direction d) {
  return d == Direction::BUY ? Direction::SELL : Direction::BUY;
}

template<class RealType>
bool is_equal(const RealType& lhs, const RealType& rhs, RealType error = RealType(1e-5)) {
  return rhs - error <= lhs && lhs <= rhs + error;
}

inline bool is_offset_open(Offset offset) {
  return offset == Offset::OPEN;
}

inline bool is_offset_close(Offset offset) {
  return offset == Offset::CLOSE ||
         offset == Offset::CLOSE_TODAY ||
         offset == Offset::CLOSE_YESTERDAY;
}

inline const std::string& to_string(Direction d) {
  static const std::map<Direction, std::string> d_str = {
    {Direction::BUY, "Buy"},
    {Direction::SELL, "Sell"}
  };

  return d_str.find(d)->second;
}

inline const std::string& to_string(OrderType t) {
  static const std::map<OrderType, std::string> t_str = {
    {OrderType::LIMIT, "Limit"},
    {OrderType::MARKET, "Market"},
    {OrderType::BEST, "Best"}
  };

  return t_str.find(t)->second;
}

inline const std::string& to_string(OrderStatus s) {
  static const std::map<OrderStatus, std::string> s_str = {
    {OrderStatus::SUBMITTING,       "Submitting"},
    {OrderStatus::REJECTED,         "Rejected"},
    {OrderStatus::NO_TRADED,        "No traded"},
    {OrderStatus::PART_TRADED,      "Part traded"},
    {OrderStatus::ALL_TRADED,       "All traded"},
    {OrderStatus::CANCELED,         "Canceled"},
    {OrderStatus::CANCEL_REJECTED,  "Cancel rejected"}
  };

  return s_str.find(s)->second;
}

inline const std::string& to_string(Offset off) {
  static const std::map<Offset, std::string> off_str = {
    {Offset::OPEN,            "Open"},
    {Offset::CLOSE,           "Close"},
    {Offset::CLOSE_TODAY,     "CloseToday"},
    {Offset::CLOSE_YESTERDAY, "CloseYesterday"}
  };

  return off_str.find(off)->second;
}

inline const std::string& to_string(ProductType product) {
  static const std::map<ProductType, std::string> product_str = {
    {ProductType::FUTURES, "Futures"},
    {ProductType::OPTIONS, "Options"}
  };

  return product_str.find(product)->second;
}

inline ProductType string2product(const std::string& product) {
  static const std::map<std::string, ProductType> product_map = {
    {"Futures", ProductType::FUTURES},
    {"Options", ProductType::OPTIONS}
  };

  return product_map.find(product)->second;
}

}  // namespace ft

#endif  // FT_INCLUDE_COMMON_H_
