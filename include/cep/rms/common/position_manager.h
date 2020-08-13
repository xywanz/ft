// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_SRC_RISK_MANAGEMENT_FUTURES_POSITION_MANAGER_H_
#define FT_SRC_RISK_MANAGEMENT_FUTURES_POSITION_MANAGER_H_

#include <map>

#include "cep/oms/portfolio.h"
#include "cep/rms/risk_rule.h"

namespace ft {

class PositionManager : public RiskRule {
 public:
  bool init(RiskRuleParams* params) override;

  int check_order_req(const Order* order) override;

  void on_order_sent(const Order* order) override;

  void on_order_traded(const Order* order, const Trade* trade) override;

  void on_order_canceled(const Order* order, int canceled) override;

  void on_order_rejected(const Order* order, int error_code) override;

 private:
  Portfolio* portfolio_;
};

};  // namespace ft

#endif  // FT_SRC_RISK_MANAGEMENT_FUTURES_POSITION_MANAGER_H_
