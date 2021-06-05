// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_INCLUDE_FT_BASE_TRADE_MSG_H_
#define FT_INCLUDE_FT_BASE_TRADE_MSG_H_

#include <cstdint>
#include <string>

#include "ft/base/error_code.h"

namespace ft {

// 交易所的英文简称
namespace exchange {
// 上海期货交易所
constexpr char kSHFE[] = "SHFE";

// 上海国际能源交易所
constexpr char kINE[] = "INE";

// 中国金融期货交易所
constexpr char kCFFEX[] = "CFFEX";

// 郑州商品交易所
constexpr char kCZCE[] = "CZCE";

// 大连商品交易所
constexpr char kDCE[] = "DCE";

// 上海证券交易所-A股
constexpr char kSSE[] = "SH";

// 深圳证券交易所-A股
constexpr char kSZE[] = "SZ";
}  // namespace exchange

enum class ProductType {
  kFutures = 0,
  kOptions,
  kStock,
  kFund,
  kUnknown,
};

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
  kBuy = 1,   // 买入
  kSell = 2,  // 卖出
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

enum class OrderStatus : uint8_t {
  kSubmitting = 0,
  kRejected,
  kAccepted,
  kPartTraded,
  kAllTraded,
  kCanceled,
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

// total_asset = cash + margin + fronzen
// balance = cash + margin + fronzen - floating_pnl
// total_asset: 总资产
// cash: 可用资金，可用于购买证券资产的资金
// margin: 保证金，对于股票来说就是持有的股票资产
// fronzen: 冻结资金，未成交的订单也需要预先占用资金
struct Account {
  uint64_t account_id;  // 资金账户号
  double total_asset;   // 总资产
  double balance;       // 结余
  double cash;          // 可用资金
  double margin;        // 保证金
  double frozen;        // 冻结金额
  double floating_pnl;  // 浮动盈亏
} __attribute__((__aligned__(8)));

struct PositionDetail {
  int yd_holdings = 0;
  int holdings = 0;
  int frozen = 0;
  int open_pending = 0;
  int close_pending = 0;

  double cost_price = 0;
  double float_pnl = 0;
} __attribute__((__aligned__(8)));

struct Position {
  uint32_t ticker_id = 0;
  PositionDetail long_pos{};
  PositionDetail short_pos{};
} __attribute__((__aligned__(8)));

struct HistoricalTrade {
  uint64_t timestamp_us;
  uint64_t order_sys_id;
  uint32_t ticker_id;
  Direction direction;
  Offset offset;
  double price;
  int volume;
};

struct HistoricalOrder {
  uint32_t ticker_id;
  Direction direction;
  Offset offset;
  int volume;
  double price;
  uint64_t order_sys_id;
};

// 策略的名称类型，用于订阅回报消息
using StrategyIdType = char[16];

// 下面是strategy和trading_server之间的交互协议
// strategy通过IPC向trading_server发送交易相关指令
// 用于简单验证交易协议的合法性
inline const uint32_t kTradingCmdMagic = 0x1709394;

// 交易指令类型
enum TraderCmdType : uint32_t {
  kNewOrder = 1,
  kCancelOrder,
  kCancelTicker,
  kCancelAll,
  kNotify,
};

// 订单请求
struct TraderOrderReq {
  uint32_t client_order_id;
  uint32_t ticker_id;
  Direction direction;
  Offset offset;
  OrderType type;
  int volume;
  double price;

  OrderFlag flags;
};

// 撤单请求
struct TraderCancelReq {
  uint64_t order_id;
};

// 撤单请求
struct TraderCancelTickerReq {
  uint32_t ticker_id;
};

// 通知
struct TraderNotification {
  uint64_t signal;
};

// 交易指令
struct TraderCommand {
  uint32_t magic;
  TraderCmdType type;
  uint64_t timestamp_us;
  bool without_check;
  StrategyIdType strategy_id;
  union {
    TraderOrderReq order_req;
    TraderCancelReq cancel_req;
    TraderCancelTickerReq cancel_ticker_req;
    TraderNotification notification;
  };
} __attribute__((__aligned__(8)));

// 订单回报
struct OrderResponse {
  uint32_t client_order_id;
  uint64_t order_id;
  uint32_t ticker_id;
  Direction direction;
  Offset offset;
  double price;
  int original_volume;
  int traded_volume;

  bool completed;
  ErrorCode error_code;
  uint32_t this_traded;
  double this_traded_price;
} __attribute__((__aligned__(8)));

}  // namespace ft

#endif  // FT_INCLUDE_FT_BASE_TRADE_MSG_H_
