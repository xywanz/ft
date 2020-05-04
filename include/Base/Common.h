// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_INCLUDE_BASE_COMMON_H_
#define FT_INCLUDE_BASE_COMMON_H_

#include <fmt/format.h>

#include <map>
#include <memory>
#include <string>

namespace ft {

inline const std::string kSHFE = "SHFE";
inline const std::string kINE = "INE";
inline const std::string kCFFEX = "CFFEX";
inline const std::string kCZCE = "CZCE";
inline const std::string kDCE = "DCE";

// such as rb2009.SHFE
inline std::string to_ticker(const std::string& symbol,
                             const std::string& exchange) {
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

#define PRINT_ORDER(logfn, ptr, msg)                                           \
  logfn("[{}] " msg                                                            \
        " <Order: \\{Ticker: {}, OrderID: {}, Direction: {}, "                 \
        "Offset: {}, OrderType: {}, Traded: {}, Total: {}, Price: {:.2f}, "    \
        "Status: {}\\}>",                                                      \
        __func__, (ptr)->ticker, (ptr)->order_id, to_string((ptr)->direction), \
        to_string((ptr)->offset), to_string((ptr)->type),                      \
        (ptr)->volume_traded, (ptr)->volume, (ptr)->price,                     \
        to_string((ptr)->status))

}  // namespace ft

#endif  // FT_INCLUDE_BASE_COMMON_H_
