// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_SRC_RISK_MANAGEMENT_FUND_MANAGER_H_
#define FT_SRC_RISK_MANAGEMENT_FUND_MANAGER_H_

#include "common/order.h"
#include "core/account.h"
#include "risk_management/risk_rule_interface.h"

namespace ft {

class FundManager : public RiskRuleInterface {
 public:
  explicit FundManager(Account* account);

  int check_order_req(const Order* order) override;

  void on_order_sent(const Order* order) override;

  void on_order_traded(const Order* order, int traded,
                       double traded_price) override;

  void on_order_canceled(const Order* order, int canceled) override;

  void on_order_rejected(const Order* order, int error_code) override;

 private:
  Account* account_{nullptr};
};

}  // namespace ft

#endif  // FT_SRC_RISK_MANAGEMENT_FUND_MANAGER_H_
