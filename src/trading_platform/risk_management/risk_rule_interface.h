// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_SRC_RISK_MANAGEMENT_RISK_RULE_INTERFACE_H_
#define FT_SRC_RISK_MANAGEMENT_RISK_RULE_INTERFACE_H_

#include <map>
#include <string>

#include "common/md_snapshot.h"
#include "common/order.h"
#include "common/portfolio.h"
#include "common/types.h"
#include "core/account.h"
#include "core/config.h"
#include "core/error_code.h"
#include "interface/trading_engine_interface.h"

namespace ft {

class RiskRuleInterface {
 public:
  virtual ~RiskRuleInterface() {}

  virtual bool init(const Config& config, Account* account,
                    Portfolio* portfolio, OrderMap* order_map,
                    const MdSnapshot* md_snapshot) {
    return true;
  }

  virtual int check_order_req(const Order* order) { return NO_ERROR; }

  virtual void on_order_sent(const Order* order) {}

  virtual void on_order_accepted(const Order* order) {}

  virtual void on_order_traded(const Order* order,
                               const OrderTradedRsp* trade) {}

  virtual void on_order_canceled(const Order* order, int canceled) {}

  virtual void on_order_completed(const Order* order) {}

  virtual void on_order_rejected(const Order* order, int error_code) {}
};

}  // namespace ft

#endif  // FT_SRC_RISK_MANAGEMENT_RISK_RULE_INTERFACE_H_
