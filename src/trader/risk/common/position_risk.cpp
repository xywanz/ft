// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "trader/risk/common/position_risk.h"

#include "ft/base/contract_table.h"
#include "ft/base/log.h"
#include "ft/utils/protocol_utils.h"

namespace ft {

bool PositionRisk::Init(RiskRuleParams* params) {
  pos_manager_ = params->pos_manager;
  LOG_INFO("position risk inited");
  return true;
}

ErrorCode PositionRisk::CheckOrderRequest(const Order& order) {
  auto& req = order.req;
  if (IsOffsetClose(req.offset)) {
    int available = 0;
    auto pos = pos_manager_->GetPosition(order.strategy_id, req.contract->ticker_id);

    if (pos) {
      auto& detail = (req.direction == Direction::kBuy ? pos->short_pos : pos->long_pos);
      available = detail.holdings - detail.close_pending;
    }

    if (available < req.volume) {
      LOG_ERROR(
          "[PositionRisk::CheckOrderRequest] no enough volume to close. "
          "Available: {}, OrderVolume: {}, OrderType: {}, {}{}",
          available, req.volume, ToString(req.type), ToString(req.direction), ToString(req.offset));
      return ErrorCode::kPositionNotEnough;
    }
  }

  return ErrorCode::kNoError;
}

void PositionRisk::OnOrderSent(const Order& order) {
  pos_manager_->UpdatePending(order.strategy_id, order.req.contract->ticker_id, order.req.direction,
                              order.req.offset, order.req.volume);
}

void PositionRisk::OnOrderTraded(const Order& order, const OrderTradedRsp& trade) {
  pos_manager_->UpdateTraded(order.strategy_id, order.req.contract->ticker_id, order.req.direction,
                             order.req.offset, trade.volume, trade.price);
}

void PositionRisk::OnOrderCanceled(const Order& order, int canceled) {
  pos_manager_->UpdatePending(order.strategy_id, order.req.contract->ticker_id, order.req.direction,
                              order.req.offset, 0 - canceled);
}

void PositionRisk::OnOrderRejected(const Order& order, ErrorCode error_code) {
  pos_manager_->UpdatePending(order.strategy_id, order.req.contract->ticker_id, order.req.direction,
                              order.req.offset, 0 - order.req.volume);
}

REGISTER_RISK_RULE("ft.risk.position", PositionRisk);

}  // namespace ft
