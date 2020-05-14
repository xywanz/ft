// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_INCLUDE_RISKMANAGEMENT_JUSTONEDIRECTION_H_
#define FT_INCLUDE_RISKMANAGEMENT_JUSTONEDIRECTION_H_

#include <string>

#include "Core/Position.h"
#include "RiskManagement/RiskRuleInterface.h"

namespace ft {

// 不可以持有双向仓位
class JustOneDirectionRule : public RiskRuleInterface {
 public:
  bool check(const OrderReq* order) override {
    const auto pos = portfolio_->get_position(order->ticker);
    const PositionDetail* opp_pos_detail;
    if (order->direction == Direction::BUY)
      opp_pos_detail = &pos.short_pos;
    else
      opp_pos_detail = &pos.long_pos;

    return opp_pos_detail->open_pending == 0 &&
           opp_pos_detail->close_pending == 0 && opp_pos_detail->volume == 0 &&
           opp_pos_detail->frozen == 0;
  }

 private:
  Portfolio* portfolio_;
};

}  // namespace ft

#endif  // FT_INCLUDE_RISKMANAGEMENT_JUSTONEDIRECTION_H_
