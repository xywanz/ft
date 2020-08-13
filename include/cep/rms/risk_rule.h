// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_SRC_RISK_MANAGEMENT_RISK_RULE_INTERFACE_H_
#define FT_SRC_RISK_MANAGEMENT_RISK_RULE_INTERFACE_H_

#include <map>
#include <string>

#include "cep/data/account.h"
#include "cep/data/config.h"
#include "cep/data/error_code.h"
#include "cep/data/md_snapshot.h"
#include "cep/data/order.h"
#include "cep/interface/oms_interface.h"
#include "cep/oms/portfolio.h"
#include "cep/rms/types.h"

namespace ft {

struct RiskRuleParams {
  const Config* config;
  const MdSnapshot* md_snapshot;
  Account* account;
  Portfolio* portfolio;
  OrderMap* order_map;
};

class RiskRule {
 public:
  virtual ~RiskRule() {}

  virtual bool init(RiskRuleParams* params) { return true; }

  virtual int check_order_req(const Order* order) { return NO_ERROR; }

  virtual void on_order_sent(const Order* order) {}

  virtual void on_order_accepted(const Order* order) {}

  virtual void on_order_traded(const Order* order, const Trade* trade) {}

  virtual void on_order_canceled(const Order* order, int canceled) {}

  virtual void on_order_completed(const Order* order) {}

  virtual void on_order_rejected(const Order* order, int error_code) {}
};

}  // namespace ft

#endif  // FT_SRC_RISK_MANAGEMENT_RISK_RULE_INTERFACE_H_
