// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "trader/risk/common/position_manager.h"

#include "ft/base/contract_table.h"
#include "ft/base/log.h"
#include "ft/utils/protocol_utils.h"

namespace ft {

bool PositionManager::Init(RiskRuleParams* params) {
  pos_calculator_ = params->pos_calculator;
  return true;
}

int PositionManager::CheckOrderRequest(const Order& order) {
  auto& req = order.req;
  if (IsOffsetClose(req.offset)) {
    int available = 0;
    auto pos = pos_calculator_->GetPosition(req.contract->ticker_id);

    if (pos) {
      auto& detail = (req.direction == Direction::kBuy ? pos->short_pos : pos->long_pos);
      available = detail.holdings - detail.close_pending;
    }

    if (available < req.volume) {
      LOG_ERROR(
          "[PositionManager::CheckOrderRequest] no enough volume to close. "
          "Available: {}, OrderVolume: {}, OrderType: {}, {}{}",
          available, req.volume, ToString(req.type), ToString(req.direction), ToString(req.offset));
      return ERR_POSITION_NOT_ENOUGH;
    }
  }

  return NO_ERROR;
}

void PositionManager::OnOrderSent(const Order& order) {
  pos_calculator_->UpdatePending(order.req.contract->ticker_id, order.req.direction,
                                 order.req.offset, order.req.volume);
}

void PositionManager::OnOrderTraded(const Order& order, const Trade& trade) {
  pos_calculator_->UpdateTraded(order.req.contract->ticker_id, order.req.direction,
                                order.req.offset, trade.volume, trade.price);
}

void PositionManager::OnOrderCanceled(const Order& order, int canceled) {
  pos_calculator_->UpdatePending(order.req.contract->ticker_id, order.req.direction,
                                 order.req.offset, 0 - canceled);
}

void PositionManager::OnOrderRejected(const Order& order, int error_code) {
  if (error_code <= ERR_SEND_FAILED) return;

  pos_calculator_->UpdatePending(order.req.contract->ticker_id, order.req.direction,
                                 order.req.offset, 0 - order.req.volume);
}

}  // namespace ft
