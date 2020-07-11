// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_INCLUDE_CORE_CONSTANTS_H_
#define FT_INCLUDE_CORE_CONSTANTS_H_

#include <map>
#include <memory>
#include <string>

namespace ft {

/*
 * 下列是交易所的英文简称
 */

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

/*
 * 订单价格类型
 * 订单价格类型还需要继续细分
 */
namespace OrderType {

// 市价单，不同的平台对市价单的支持不同
inline const uint32_t MARKET = 1;

// 限价单，指定价格的挂单，订单时限为当日有效
inline const uint32_t LIMIT = 2;

// 对方最优价格的限价单
inline const uint32_t BEST = 3;

// 立即全部成交否则立即撤销，不同平台的实现也不同
// 对于CTP来说是限价单，需要指定价格
inline const uint32_t FAK = 4;

// 立即成交剩余部分立即撤单，不同平台的实现也不同
// 对于CTP来说是限价单，需要指定价格
inline const uint32_t FOK = 5;

// 下面是港交所的订单类型
namespace hkex {

// 竞价盘
inline const uint32_t MO_AT_CROSSING = 10;

// 竞价现价盘
inline const uint32_t LO_AT_CROSSING = 11;

// 限价盘
inline const uint32_t LO = 12;

// 增强现价盘
inline const uint32_t ELO = 13;

// 特殊限价盘
inline const uint32_t SLO = 14;

}  // namespace hkex

}  // namespace OrderType

/*
 * 交易的方向
 */
namespace Direction {
// 买入
inline const uint32_t BUY = 1;

// 卖出
inline const uint32_t SELL = 2;

// 申购，暂时不支持
inline const uint32_t PURCHASE = 4;

// 赎回，暂时不支持
inline const uint32_t REDEEM = 5;
}  // namespace Direction

/*
 * 开平类型
 * 不是所有金融产品的交易都需要开平类型
 * 例如A股普通股票的买卖就只涉及交易方向而不涉及开平类型
 */
namespace Offset {
// 开仓
inline const uint32_t OPEN = 1;

// 平仓
inline const uint32_t CLOSE = 2;

// 平今
inline const uint32_t CLOSE_TODAY = 4;

// 平昨
inline const uint32_t CLOSE_YESTERDAY = 8;
}  // namespace Offset

namespace TradeType {

// 二级市场买卖
inline const uint32_t SECONDARY_MARKET = 0;

// 一级市场成交，如申赎完成会返回此类型
inline const uint32_t PRIMARY_MARKET = 1;

// ETF申赎的现金替代，也会以回报的形式通知
inline const uint32_t CASH_SUBSTITUTION = 2;

// ETF赎回获得股票，获得成分股的回报
inline const uint32_t ACQUIRED_STOCK = 3;

// ETF申购消耗股票，消耗成分股的回报
inline const uint32_t RELEASED_STOCK = 4;

}  // namespace TradeType

/*
 * 对手方，只针买卖方向有效
 */
inline uint32_t opp_direction(uint32_t d) {
  return d == Direction::BUY ? Direction::SELL : Direction::BUY;
}

/*
 * 是否是开仓
 */
inline bool is_offset_open(uint32_t offset) { return offset == Offset::OPEN; }

/*
 * 是否是平仓，平仓包括平仓、平今、平昨
 */
inline bool is_offset_close(uint32_t offset) {
  return offset &
         (Offset::CLOSE | Offset::CLOSE_TODAY | Offset::CLOSE_YESTERDAY);
}

namespace internal {
static inline const std::string __empty_str = "";
}
/*
 * 交易方向转为string
 */
inline const std::string& direction_str(uint32_t d) {
  static const std::string d_str[] = {"Unknown", "Buy",      "Sell",
                                      "Unknown", "Purchase", "Redeem"};

  if (d > Direction::REDEEM) return internal::__empty_str;
  return d_str[d];
}

/*
 * 开平类型转为string
 */
inline const std::string& offset_str(uint32_t off) {
  static const std::string off_str[] = {
      "Unknown", "Open",    "Close",   "Unknown",       "CloseToday",
      "Unknown", "Unknown", "Unknown", "CloseYesterday"};

  if (off > Offset::CLOSE_YESTERDAY) return internal::__empty_str;
  return off_str[off];
}

/*
 * 订单价格类型转string
 */
inline const std::string& ordertype_str(uint32_t t) {
  static const std::string t_str[] = {"Unknown", "Market", "Limit",
                                      "Best",    "FAK",    "FOK"};

  if (t > OrderType::FOK) return internal::__empty_str;
  return t_str[t];
}

/*
 * 判断浮点数是否相等
 */
template <class RealType>
bool is_equal(const RealType& lhs, const RealType& rhs,
              RealType error = RealType(1e-5)) {
  return rhs - error <= lhs && lhs <= rhs + error;
}

}  // namespace ft

#endif  // FT_INCLUDE_CORE_CONSTANTS_H_
