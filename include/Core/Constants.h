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
inline const std::string EX_SHFE = "SHFE";

// 上海国际能源交易所
inline const std::string EX_INE = "INE";

// 中国金融期货交易所
inline const std::string EX_CFFEX = "CFFEX";

// 郑州商品交易所
inline const std::string EX_CZCE = "CZCE";

// 大连商品交易所
inline const std::string EX_DCE = "DCE";

// 上海证券交易所-A股
inline const std::string EX_SH_A = "SH";

// 深圳证券交易所-A股
inline const std::string EX_SZ_A = "SZ";

/*
 * 订单价格类型
 * 订单价格类型还需要继续细分
 */
namespace OrderType {
// 限价单，指定价格的挂单，订单时限为当日有效
inline const uint32_t LIMIT = 1;

// 市价单，不同的平台对市价单的支持不同
inline const uint32_t MARKET = 2;

// 对方最优价格的限价单
inline const uint32_t BEST = 3;

// 立即全部成交否则立即撤销，不同平台的实现也不同
// 对于CTP来说是限价单，需要指定价格
inline const uint32_t FAK = 4;

// 立即成交剩余部分立即撤单，不同平台的实现也不同
// 对于CTP来说是限价单，需要指定价格
inline const uint32_t FOK = 5;
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

/*
 * 交易方向转为string
 */
inline const std::string& direction_str(uint32_t d) {
  static const std::map<uint32_t, std::string> d_str = {
      {Direction::BUY, "Buy"}, {Direction::SELL, "Sell"}};

  return d_str.find(d)->second;
}

/*
 * 开平类型转为string
 */
inline const std::string& offset_str(uint32_t off) {
  static const std::map<uint32_t, std::string> off_str = {
      {Offset::OPEN, "Open"},
      {Offset::CLOSE, "Close"},
      {Offset::CLOSE_TODAY, "CloseToday"},
      {Offset::CLOSE_YESTERDAY, "CloseYesterday"}};

  return off_str.find(off)->second;
}

/*
 * 订单价格类型转string
 */
inline const std::string& ordertype_str(uint32_t t) {
  static const std::map<uint32_t, std::string> t_str = {
      {OrderType::LIMIT, "Limit"},
      {OrderType::MARKET, "Market"},
      {OrderType::BEST, "Best"},
      {OrderType::FAK, "FAK"},
      {OrderType::FOK, "FOK"}};

  return t_str.find(t)->second;
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
