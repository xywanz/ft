// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_INCLUDE_RISKMANAGEMENT_RISKMANAGER_H_
#define FT_INCLUDE_RISKMANAGEMENT_RISKMANAGER_H_

#include <list>
#include <memory>
#include <string>

#include "Core/Gateway.h"
#include "RiskManagement/RiskRuleInterface.h"

namespace ft {

class RiskManager {
 public:
  void add_rule(std::shared_ptr<RiskRuleInterface> rule);

  // 返回false则拦截订单
  bool check(const OrderReq* order);

 private:
  std::list<std::shared_ptr<RiskRuleInterface>> rules_;
};

}  // namespace ft

#endif  // FT_INCLUDE_RISKMANAGEMENT_RISKMANAGER_H_
