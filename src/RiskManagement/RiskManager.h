// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_SRC_RISKMANAGEMENT_RISKMANAGER_H_
#define FT_SRC_RISKMANAGEMENT_RISKMANAGER_H_

#include <list>
#include <memory>
#include <string>

#include "Common/PositionManager.h"
#include "Core/Gateway.h"
#include "Core/RiskManagementInterface.h"
#include "RiskManagement/RiskRuleInterface.h"

namespace ft {

class RiskManager : public RiskManagementInterface {
 public:
  explicit RiskManager(const PositionManager* pos_mgr);

  void add_rule(std::shared_ptr<RiskRuleInterface> rule);

  bool check_order_req(const OrderReq* req) override;

  void on_order_sent(uint64_t engine_order_id) override;

  void on_order_traded(uint64_t engine_order_id, int this_traded,
                       double traded_price) override;

  void on_order_completed(uint64_t engine_order_id) override;

 private:
  const PositionManager* pos_mgr_;
  std::list<std::shared_ptr<RiskRuleInterface>> rules_;
};

}  // namespace ft

#endif  // FT_SRC_RISKMANAGEMENT_RISKMANAGER_H_
