// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_SRC_RISK_MANAGEMENT_NO_SELF_TRADE_H_
#define FT_SRC_RISK_MANAGEMENT_NO_SELF_TRADE_H_

#include <spdlog/spdlog.h>

#include <map>
#include <string>

#include "risk_management/risk_rule_interface.h"

namespace ft {

// 拦截自成交订单，检查相反方向的挂单
// 1. 市价单
// 2. 非市价单的其他订单，且价格可以成功撮合的
class NoSelfTradeRule : public RiskRuleInterface {
 public:
  explicit NoSelfTradeRule(std::map<uint64_t, Order>* order_map);

  int check_order_req(const Order* req) override;

 private:
  std::map<uint64_t, Order>* order_map_;
};

}  // namespace ft

#endif  // FT_SRC_RISK_MANAGEMENT_NO_SELF_TRADE_H_
