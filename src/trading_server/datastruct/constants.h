// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_SRC_TRADING_SERVER_DATASTRUCT_CONSTANTS_H_
#define FT_SRC_TRADING_SERVER_DATASTRUCT_CONSTANTS_H_

#include <map>
#include <memory>
#include <string>

namespace ft {

// 交易所的英文简称
// 上海期货交易所
inline const std::string SHFE = "SHFE";

// 上海国际能源交易所
inline const std::string INE = "INE";

// 中国金融期货交易所
inline const std::string CFFEX = "CFFEX";

// 郑州商品交易所
inline const std::string CZCE = "CZCE";

// 大连商品交易所
inline const std::string DCE = "DCE";

// 上海证券交易所-A股
inline const std::string SSE = "SH";

// 深圳证券交易所-A股
inline const std::string SZE = "SZ";

// 订单价格类型
enum class OrderType : uint8_t {
  // 市价单，不同的平台对市价单的支持不同
  kMarket = 1,
  // 限价单，指定价格的挂单，订单时限为当日有效
  kLimit = 2,
  // 对方最优价格的限价单
  kBest = 3,
  // 立即全部成交否则立即撤销，不同平台的实现也不同
  // 对于CTP来说是限价单，需要指定价格
  kFak = 4,
  // 立即成交剩余部分立即撤单，不同平台的实现也不同
  // 对于CTP来说是限价单，需要指定价格
  kFok = 5,
  kUnknown = 6,

  // 下面是港交所的订单类型
  // 竞价盘
  kHKEX_MO_AT_CROSSING = 10,
  // 竞价现价盘
  kHKEX_LO_AT_CROSSING = 11,
  // 限价盘
  kHKEX_LO = 12,
  // 增强现价盘
  kHKEX_ELO = 13,
  // 特殊限价盘
  kHKEX_SLO = 14,
};

// 交易的方向
enum class Direction : uint8_t {
  kBuy = 1,       // 买入
  kSell = 2,      // 卖出
  kPurchase = 4,  // 申购
  kRedeem = 5,    // 赎回
  kUnknown = 6,
};

/*
 * 开平类型
 * 不是所有金融产品的交易都需要开平类型
 * 例如A股普通股票的买卖就只涉及交易方向而不涉及开平类型
 */
enum class Offset : uint8_t {
  kOpen = 1,
  kClose = 2,
  kCloseToday = 4,
  kCloseYesterday = 8,
  kOffsetNone = 9,
  kUnknown = 10,
};

/*
 * 订单的一些标志位，如CTP的套保等
 */
enum OrderFlag : uint8_t {
  kNone = 0x0,
  kHedge = 0x1,  // 套保标志
};

enum class TradeType {
  kSecondaryMarket = 0,   // 二级市场买卖
  kPrimaryMarket = 1,     // 一级市场成交，如申赎完成会返回此类型
  kCashSubstitution = 2,  // ETF申赎的现金替代，也会以回报的形式通知
  kAcquireStock = 3,      // ETF赎回获得股票，获得成分股的回报
  kReleaseStock = 4,      // ETF申购消耗股票，消耗成分股的回报
  kUnknown = 5,
};

/*
 * 对手方，只针买卖方向有效
 */
inline Direction OppositeDirection(Direction direction) {
  return direction == Direction::kBuy ? Direction::kSell : Direction::kBuy;
}

/*
 * 是否是开仓
 */
inline bool IsOffsetOpen(Offset offset) { return offset == Offset::kOpen; }

/*
 * 是否是平仓，平仓包括平仓、平今、平昨
 */
inline bool IsOffsetClose(Offset offset) {
  return static_cast<uint8_t>(offset) &
         (static_cast<uint8_t>(Offset::kClose) | static_cast<uint8_t>(Offset::kCloseToday) |
          static_cast<uint8_t>(Offset::kCloseYesterday));
}

/*
 * 交易方向转为string
 */
inline std::string DirectionToStr(Direction direction) {
  static const char* direction_str[] = {"Unknown", "Buy", "Sell", "Unknown", "Purchase", "Redeem"};

  auto index = static_cast<uint8_t>(direction);
  if (index >= static_cast<uint8_t>(Direction::kUnknown)) {
    return "Unknown";
  }
  return direction_str[index];
}

/*
 * 开平类型转为string
 */
inline std::string OffsetToStr(Offset offset) {
  static const char* offset_str[] = {"Unknown", "Open",    "Close",   "Unknown",       "CloseToday",
                                     "Unknown", "Unknown", "Unknown", "CloseYesterday"};

  auto index = static_cast<uint8_t>(offset);
  if (index > static_cast<uint8_t>(Offset::kUnknown)) {
    return "Unknown";
  }
  return offset_str[index];
}

/*
 * 订单价格类型转string
 */
inline std::string OrderTypeToStr(OrderType order_type) {
  static const char* order_type_str[] = {"Unknown", "Market", "Limit", "Best", "FAK", "FOK"};

  auto index = static_cast<uint8_t>(order_type);
  if (index > static_cast<uint8_t>(OrderType::kUnknown)) {
    return "Unknown";
  }
  return order_type_str[index];
}

/*
 * 判断浮点数是否相等
 */
template <class RealType>
bool IsEqual(const RealType& lhs, const RealType& rhs, RealType error = RealType(1e-5)) {
  return rhs - error <= lhs && lhs <= rhs + error;
}

}  // namespace ft

#endif  // FT_SRC_TRADING_SERVER_DATASTRUCT_CONSTANTS_H_
