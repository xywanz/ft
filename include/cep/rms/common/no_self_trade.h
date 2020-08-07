// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_SRC_RISK_MANAGEMENT_COMMON_NO_SELF_TRADE_H_
#define FT_SRC_RISK_MANAGEMENT_COMMON_NO_SELF_TRADE_H_

#include <spdlog/spdlog.h>

#include <map>
#include <string>

#include "cep/rms/risk_rule.h"

namespace ft {

// 拦截自成交订单，检查相反方向的挂单
// 1. 市价单
// 2. 非市价单的其他订单，且价格可以成功撮合的
class NoSelfTradeRule : public RiskRule {
 public:
  bool init(const Config& config, Account* account, Portfolio* portfolio,
            OrderMap* order_map, const MdSnapshot* md_snapshotp) override;

  int check_order_req(const Order* req) override;

 private:
  OrderMap* order_map_;
};

}  // namespace ft

#endif  // FT_SRC_RISK_MANAGEMENT_COMMON_NO_SELF_TRADE_H_
