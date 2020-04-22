// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_INCLUDE_RISKMANAGEMENT_RISKRULEINTERFACE_H_
#define FT_INCLUDE_RISKMANAGEMENT_RISKRULEINTERFACE_H_

#include <string>

#include "Common.h"
#include "Order.h"

namespace ft {

class RiskRuleInterface {
 public:
  virtual ~RiskRuleInterface() {}

  // 返回false则拦截订单
  virtual bool check(const Order* order) = 0;
};

}  // namespace ft

#endif  // FT_INCLUDE_RISKMANAGEMENT_RISKRULEINTERFACE_H_
