// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_SRC_CEP_RMS_RMS_H_
#define FT_SRC_CEP_RMS_RMS_H_

#include <list>
#include <map>
#include <memory>
#include <string>

#include "cep/data/account.h"
#include "cep/data/config.h"
#include "cep/data/md_snapshot.h"
#include "cep/data/order.h"
#include "cep/interface/oms_interface.h"
#include "cep/oms/portfolio.h"
#include "cep/rms/risk_rule.h"
#include "cep/rms/types.h"

namespace ft {

class RMS {
 public:
  RMS();

  bool init(const Config& config, Account* account, Portfolio* portfolio,
            OrderMap* order_map, const MdSnapshot* md_snapshot);

  void add_rule(std::shared_ptr<RiskRule> rule);

  int check_order_req(const Order* order);

  void on_order_sent(const Order* order);

  void on_order_accepted(const Order* order);

  void on_order_traded(const Order* order, const Trade* trade);

  void on_order_canceled(const Order* order, int canceled);

  void on_order_rejected(const Order* order, int error_code);

  void on_order_completed(const Order* order);

 private:
  std::list<std::shared_ptr<RiskRule>> rules_;
};

}  // namespace ft

#endif  // FT_SRC_CEP_RMS_RMS_H_
