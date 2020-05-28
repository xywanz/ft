// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_SRC_RISKMANAGEMENT_NOSELFTRADE_H_
#define FT_SRC_RISKMANAGEMENT_NOSELFTRADE_H_

#include <spdlog/spdlog.h>

#include <string>
#include <vector>

#include "RiskManagement/RiskRuleInterface.h"

namespace ft {

// 拦截自成交订单，检查相反方向的挂单
// 1. 市价单
// 2. 非市价单的其他订单，且价格可以成功撮合的
class NoSelfTradeRule : public RiskRuleInterface {
 public:
  int check_order_req(const Order* req) override;

  void on_order_completed(const Order* order) override;

 private:
  std::vector<Order> orders_;
};

}  // namespace ft

#endif  // FT_SRC_RISKMANAGEMENT_NOSELFTRADE_H_
