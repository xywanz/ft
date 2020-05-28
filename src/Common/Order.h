// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_SRC_COMMON_ORDER_H_
#define FT_SRC_COMMON_ORDER_H_

#include <map>
#include <string>

#include "Core/Constants.h"
#include "Core/Contract.h"
#include "Core/Protocol.h"

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
  OrderReq req;

  /*
   * 这个ID是给风控使用的，因为风控在发单前需要检查订单，如果后续发单失败，
   * 需要通过某种途径通知风控模块该订单对应于之前的哪个请求。Gateway返回的
   * 订单号要在发单之后才能知道，而风控在发单前就需要把订单能一一对应起来，
   * 于是TradingEngine会维护这个engine_order_id
   */
  uint64_t engine_order_id;

  /*
   * 这个ID是策略发单的时候提供的，使策略能对应其订单回报
   */
  uint32_t user_order_id;

  const Contract* contract = nullptr;
  int traded_volume = 0;
  int canceled_volume = 0;
  OrderStatus status;
  uint64_t insert_time;
  std::string strategy_id;
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

#endif  // FT_SRC_COMMON_ORDER_H_
