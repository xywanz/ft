// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_INCLUDE_FT_UTILS_PROTOCOL_UTILS_H_
#define FT_INCLUDE_FT_UTILS_PROTOCOL_UTILS_H_

#include <string>

#include "fmt/format.h"
#include "ft/base/contract_table.h"
#include "ft/base/trade_msg.h"

namespace ft {

// 对手方，只针买卖方向有效
inline Direction OppositeDirection(Direction direction) {
  return direction == Direction::kBuy ? Direction::kSell : Direction::kBuy;
}

// 是否是开仓
inline bool IsOffsetOpen(Offset offset) { return offset == Offset::kOpen; }

// 是否是平仓，平仓包括平仓、平今、平昨
inline bool IsOffsetClose(Offset offset) {
  return static_cast<uint8_t>(offset) &
         (static_cast<uint8_t>(Offset::kClose) | static_cast<uint8_t>(Offset::kCloseToday) |
          static_cast<uint8_t>(Offset::kCloseYesterday));
}

inline std::string ToString(Direction direction) {
  static const char* direction_str[] = {"Unknown", "Buy", "Sell", "Unknown", "Purchase", "Redeem"};

  auto index = static_cast<uint8_t>(direction);
  if (index >= static_cast<uint8_t>(Direction::kUnknown)) {
    return "Unknown";
  }
  return direction_str[index];
}

inline std::string ToString(Offset offset) {
  static const char* offset_str[] = {"Unknown", "Open",    "Close",   "Unknown",       "CloseToday",
                                     "Unknown", "Unknown", "Unknown", "CloseYesterday"};

  auto index = static_cast<uint8_t>(offset);
  if (index > static_cast<uint8_t>(Offset::kUnknown)) {
    return "Unknown";
  }
  return offset_str[index];
}

inline std::string ToString(OrderType order_type) {
  static const char* order_type_str[] = {"Unknown", "Market", "Limit", "Best", "FAK", "FOK"};

  auto index = static_cast<uint8_t>(order_type);
  if (index > static_cast<uint8_t>(OrderType::kUnknown)) {
    return "Unknown";
  }
  return order_type_str[index];
}

inline std::string ToString(ProductType product) {
  if (product == ProductType::kFutures) return "Futures";
  if (product == ProductType::kOptions) return "Options";
  if (product == ProductType::kStock) return "Stock";
  if (product == ProductType::kFund) return "Fund";

  return "Unknown";
}

inline ProductType StringToProductType(const std::string& product) {
  if (product == "Futures") return ProductType::kFutures;
  if (product == "Options") return ProductType::kOptions;
  if (product == "Stock") return ProductType::kStock;
  if (product == "Fund") return ProductType::kFund;

  return ProductType::kUnknown;
}

inline std::string DumpPosition(const Position& pos) {
  std::string_view ticker = "";
  auto contract = ContractTable::get_by_index(pos.ticker_id);
  if (contract) ticker = contract->ticker;

  auto& lp = pos.long_pos;
  auto& sp = pos.short_pos;
  return fmt::format(
      "<Position ticker:{} |LONG holdings:{} yd_holdings:{} frozen:{} "
      "open_pending:{} close_pending:{} cost_price:{} |SHORT holdings:{} "
      "yd_holdings:{} frozen:{} open_pending:{} close_pending:{} "
      "cost_price:{}>",
      ticker, lp.holdings, lp.yd_holdings, lp.frozen, lp.open_pending, lp.close_pending,
      lp.cost_price, sp.holdings, sp.yd_holdings, sp.frozen, sp.open_pending, sp.close_pending,
      sp.cost_price);
}

}  // namespace ft

#endif  // FT_INCLUDE_FT_UTILS_PROTOCOL_UTILS_H_
