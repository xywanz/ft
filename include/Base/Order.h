// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_INCLUDE_BASE_ORDER_H_
#define FT_INCLUDE_BASE_ORDER_H_

#include <map>
#include <string>

#include "Base/Common.h"

namespace ft {

enum class OrderStatus {
  CREATED = 0,
  SUBMITTING,
  REJECTED,
  NO_TRADED,
  PART_TRADED,
  ALL_TRADED,
  CANCELED,
  CANCEL_REJECTED
};

enum class OrderType {
  LIMIT = 0,  // 市价
  MARKET,     //
  FAK,        //
  FOK,        //
  BEST,       // 最优价
};

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

struct Order {
  Order() {}

  Order(const std::string& _ticker, Direction _direction, Offset _offset,
        int _volume, OrderType _type, double _price)
      : ticker(_ticker),
        direction(_direction),
        offset(_offset),
        volume(_volume),
        type(_type),
        price(_price) {
    ticker_split(_ticker, &symbol, &exchange);
  }

  // req data
  std::string symbol;
  std::string exchange;
  std::string ticker;
  std::string order_id;
  OrderType type;
  Direction direction;
  Offset offset;
  double price = 0;
  int64_t volume = 0;

  // rsp or local data
  OrderStatus status;
  int64_t volume_traded = 0;
  std::string insert_time;
};

inline Direction opp_direction(Direction d) {
  return d == Direction::BUY ? Direction::SELL : Direction::BUY;
}

inline bool is_offset_open(Offset offset) { return offset == Offset::OPEN; }

inline bool is_offset_close(Offset offset) {
  return offset == Offset::CLOSE || offset == Offset::CLOSE_TODAY ||
         offset == Offset::CLOSE_YESTERDAY;
}

inline const std::string& to_string(Direction d) {
  static const std::map<Direction, std::string> d_str = {
      {Direction::BUY, "Buy"}, {Direction::SELL, "Sell"}};

  return d_str.find(d)->second;
}

inline const std::string& to_string(OrderType t) {
  static const std::map<OrderType, std::string> t_str = {
      {OrderType::LIMIT, "Limit"},
      {OrderType::MARKET, "Market"},
      {OrderType::BEST, "Best"}};

  return t_str.find(t)->second;
}

inline const std::string& to_string(OrderStatus s) {
  static const std::map<OrderStatus, std::string> s_str = {
      {OrderStatus::SUBMITTING, "Submitting"},
      {OrderStatus::REJECTED, "Rejected"},
      {OrderStatus::NO_TRADED, "No traded"},
      {OrderStatus::PART_TRADED, "Part traded"},
      {OrderStatus::ALL_TRADED, "All traded"},
      {OrderStatus::CANCELED, "Canceled"},
      {OrderStatus::CANCEL_REJECTED, "Cancel rejected"}};

  return s_str.find(s)->second;
}

inline const std::string& to_string(Offset off) {
  static const std::map<Offset, std::string> off_str = {
      {Offset::OPEN, "Open"},
      {Offset::CLOSE, "Close"},
      {Offset::CLOSE_TODAY, "CloseToday"},
      {Offset::CLOSE_YESTERDAY, "CloseYesterday"}};

  return off_str.find(off)->second;
}

}  // namespace ft

#endif  // FT_INCLUDE_BASE_ORDER_H_
