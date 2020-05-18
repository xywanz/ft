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

  virtual bool check_order_req(const OrderReq* req) { return true; }

  virtual void on_order_sent(uint64_t order_id) {}

  virtual void on_order_traded(uint64_t order_id, int64_t this_traded,
                               double traded_price) {}

  virtual void on_order_completed(uint64_t order_id) {}
};

}  // namespace ft

#endif  // FT_SRC_RISKMANAGEMENT_RISKRULEINTERFACE_H_
