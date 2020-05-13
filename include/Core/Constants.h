// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_INCLUDE_BASE_COMMON_H_
#define FT_INCLUDE_BASE_COMMON_H_

#include <fmt/format.h>

#include <map>
#include <memory>
#include <string>

namespace ft {

inline const std::string EX_SHFE = "SHFE";
inline const std::string EX_INE = "INE";
inline const std::string EX_CFFEX = "CFFEX";
inline const std::string EX_CZCE = "CZCE";
inline const std::string EX_DCE = "DCE";

namespace OrderType {
inline const uint64_t LIMIT = 1;
inline const uint64_t MARKET = 2;
inline const uint64_t BEST = 3;
inline const uint64_t FAK = 4;
inline const uint64_t FOK = 5;
}  // namespace OrderType

namespace Direction {
inline const uint64_t BUY = 1;
inline const uint64_t SELL = 2;
}  // namespace Direction

namespace Offset {
inline const uint64_t OPEN = 1;
inline const uint64_t CLOSE = 2;
inline const uint64_t CLOSE_TODAY = 4;
inline const uint64_t CLOSE_YESTERDAY = 8;
}  // namespace Offset

inline std::string to_ticker(std::string symbol, std::string exchange) {
  return fmt::format("{}.{}", symbol, exchange);
}

inline void ticker_split(const std::string& ticker, std::string* symbol,
                         std::string* exchange) {
  if (ticker.empty()) return;

  auto pos = ticker.find_first_of('.');
  *symbol = ticker.substr(0, pos);
  if (pos != std::string::npos && pos + 1 < ticker.size())
    *exchange = ticker.substr(pos + 1);
}

template <class RealType>
bool is_equal(const RealType& lhs, const RealType& rhs,
              RealType error = RealType(1e-5)) {
  return rhs - error <= lhs && lhs <= rhs + error;
}

}  // namespace ft

#endif  // FT_INCLUDE_BASE_COMMON_H_
