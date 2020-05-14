// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_SRC_RISKMANAGEMENT_RISKRULEINTERFACE_H_
#define FT_SRC_RISKMANAGEMENT_RISKRULEINTERFACE_H_

#include <string>

#include "Core/Gateway.h"
#include "Core/Protocol.h"

namespace ft {

class RiskRuleInterface {
 public:
  virtual ~RiskRuleInterface() {}

  // 返回false则拦截订单
  virtual bool check(const OrderReq* req) = 0;
};

}  // namespace ft

#endif  // FT_SRC_RISKMANAGEMENT_RISKRULEINTERFACE_H_
