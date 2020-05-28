// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_SRC_RISKMANAGEMENT_RISKMANAGER_H_
#define FT_SRC_RISKMANAGEMENT_RISKMANAGER_H_

#include <list>
#include <memory>
#include <string>

#include "Common/Order.h"
#include "Common/Portfolio.h"
#include "Core/Account.h"
#include "RiskManagement/RiskRuleInterface.h"

namespace ft {

class RiskManager {
 public:
  RiskManager(Account* account, Portfolio* pos_mgr);

  void add_rule(std::shared_ptr<RiskRuleInterface> rule);

  int check_order_req(const Order* order);

  void on_order_sent(const Order* order);

  void on_order_accepted(const Order* order);

  void on_order_traded(const Order* order, int this_traded,
                       double traded_price);

  void on_order_canceled(const Order* order, int canceled);

  void on_order_rejected(const Order* order, int error_code);

  void on_order_completed(const Order* order);

 private:
  std::list<std::shared_ptr<RiskRuleInterface>> rules_;
};

}  // namespace ft

#endif  // FT_SRC_RISKMANAGEMENT_RISKMANAGER_H_
