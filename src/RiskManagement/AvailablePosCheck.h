// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_SRC_RISKMANAGEMENT_AVAILABLEPOSCHECK_H_
#define FT_SRC_RISKMANAGEMENT_AVAILABLEPOSCHECK_H_

#include <spdlog/spdlog.h>

#include <string>
#include <vector>

#include "Common/PositionManager.h"
#include "RiskManagement/RiskRuleInterface.h"

namespace ft {

// 拦截自成交订单，检查相反方向的挂单
// 1. 市价单
// 2. 非市价单的其他订单，且价格可以成功撮合的
class AvailablePosCheck : public RiskRuleInterface {
 public:
  explicit AvailablePosCheck(const PositionManager* pos_mgr);

  bool check_order_req(const OrderReq* order) override;

 private:
  const PositionManager* pos_mgr_{nullptr};
};

}  // namespace ft

#endif  // FT_SRC_RISKMANAGEMENT_AVAILABLEPOSCHECK_H_
