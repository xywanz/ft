// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_SRC_RISK_MANAGEMENT_RISK_MANAGER_H_
#define FT_SRC_RISK_MANAGEMENT_RISK_MANAGER_H_

#include <list>
#include <map>
#include <memory>
#include <string>

#include "common/order.h"
#include "common/portfolio.h"
#include "core/account.h"
#include "core/config.h"
#include "risk_management/risk_rule_interface.h"

namespace ft {

class RiskManager {
 public:
  RiskManager();

  bool init(const Config& config, Account* account, Portfolio* portfolio,
            std::map<uint64_t, Order>* order_map);

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

#endif  // FT_SRC_RISK_MANAGEMENT_RISK_MANAGER_H_
