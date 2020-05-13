// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_TRADINGSYSTEM_ORDER_H_
#define FT_TRADINGSYSTEM_ORDER_H_

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

}  // namespace ft

#endif  // FT_TRADINGSYSTEM_ORDER_H_
