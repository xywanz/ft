// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_SRC_RISKMANAGEMENT_NOSELFTRADE_H_
#define FT_SRC_RISKMANAGEMENT_NOSELFTRADE_H_

#include <spdlog/spdlog.h>

#include <string>
#include <vector>

#include "RiskManagement/RiskRuleInterface.h"
#include "TradingSystem/PositionManager.h"

namespace ft {

// 拦截自成交订单，检查相反方向的挂单
// 1. 市价单
// 2. 非市价单的其他订单，且价格可以成功撮合的
class NoSelfTradeRule : public RiskRuleInterface {
 public:
  bool check_order_req(const OrderReq* order) override;

  void on_order_completed(uint64_t order_id) override;

 private:
  const PositionManager* pos_mgr_;

  std::vector<OrderReq> orders_;
};

}  // namespace ft

#endif  // FT_SRC_RISKMANAGEMENT_NOSELFTRADE_H_
