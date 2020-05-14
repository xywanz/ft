// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_SRC_RISKMANAGEMENT_RISKMANAGER_H_
#define FT_SRC_RISKMANAGEMENT_RISKMANAGER_H_

#include <list>
#include <memory>
#include <string>

#include "Core/Gateway.h"
#include "Core/RiskManagementInterface.h"
#include "RiskManagement/RiskRuleInterface.h"

namespace ft {

class RiskManager : public RiskManagementInterface {
 public:
  void add_rule(std::shared_ptr<RiskRuleInterface> rule);

  bool check_order_req(const OrderReq* req) override;

  void on_order_traded(uint64_t order_id, int64_t this_traded,
                       double traded_price) override;

  void on_order_completed(uint64_t order_id) override;

 private:
  std::list<std::shared_ptr<RiskRuleInterface>> rules_;
};

}  // namespace ft

#endif  // FT_SRC_RISKMANAGEMENT_RISKMANAGER_H_
