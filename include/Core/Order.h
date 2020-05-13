// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_INCLUDE_BASE_ORDER_H_
#define FT_INCLUDE_BASE_ORDER_H_

#include <map>
#include <string>

#include "Core/Constants.h"
#include "Core/Contract.h"

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

struct Order {
  const Contract* contract;
  uint64_t order_id;
  uint64_t type;
  uint64_t direction;
  uint64_t offset;
  double price = 0;
  int64_t volume = 0;
  int64_t traded_volume = 0;
  int64_t canceled_volume = 0;
  OrderStatus status;
  uint64_t insert_time;
};

inline uint64_t opp_direction(uint64_t d) {
  return d == Direction::BUY ? Direction::SELL : Direction::BUY;
}

inline bool is_offset_open(uint64_t offset) { return offset == Offset::OPEN; }

inline bool is_offset_close(uint64_t offset) {
  return offset &
         (Offset::CLOSE | Offset::CLOSE_TODAY | Offset::CLOSE_YESTERDAY);
}

inline const std::string& direction_str(uint64_t d) {
  static const std::map<uint64_t, std::string> d_str = {
      {Direction::BUY, "Buy"}, {Direction::SELL, "Sell"}};

  return d_str.find(d)->second;
}

inline const std::string& ordertype_str(uint64_t t) {
  static const std::map<uint64_t, std::string> t_str = {
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

inline const std::string& offset_str(uint64_t off) {
  static const std::map<uint64_t, std::string> off_str = {
      {Offset::OPEN, "Open"},
      {Offset::CLOSE, "Close"},
      {Offset::CLOSE_TODAY, "CloseToday"},
      {Offset::CLOSE_YESTERDAY, "CloseYesterday"}};

  return off_str.find(off)->second;
}

}  // namespace ft

#endif  // FT_INCLUDE_BASE_ORDER_H_
