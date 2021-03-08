// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "trader/risk_management/common/position_manager.h"

#include "ft/base/contract_table.h"
#include "ft/utils/protocol_utils.h"
#include "spdlog/spdlog.h"

namespace ft {

bool PositionManager::Init(RiskRuleParams* params) {
  pos_calculator_ = params->pos_calculator;
  return true;
}

int PositionManager::CheckOrderRequest(const Order* order) {
  if (order->req.direction != Direction::kBuy && order->req.direction != Direction::kSell)
    return NO_ERROR;

  auto* req = &order->req;
  if (IsOffsetClose(req->offset)) {
    int available = 0;
    auto pos = pos_calculator_->GetPosition(req->contract->ticker_id);

    if (pos) {
      auto& detail = (req->direction == Direction::kBuy ? pos->short_pos : pos->long_pos);
      available = detail.holdings - detail.close_pending;
    }

    if (available < req->volume) {
      spdlog::error(
          "[PositionManager::CheckOrderRequest] Not enough volume to Close. "
          "Available: {}, OrderVolume: {}, OrderType: {}, {}{}",
          available, req->volume, ToString(req->type), ToString(req->direction),
          ToString(req->offset));
      return ERR_POSITION_NOT_ENOUGH;
    }
  }

  return NO_ERROR;
}

void PositionManager::OnOrderSent(const Order* order) {
  pos_calculator_->UpdatePending(order->req.contract->ticker_id, order->req.direction,
                                 order->req.offset, order->req.volume);
}

void PositionManager::OnOrderTraded(const Order* order, const Trade* trade) {
  if (trade->trade_type == TradeType::kSecondaryMarket ||
      trade->trade_type == TradeType::kPrimaryMarket) {
    pos_calculator_->UpdateTraded(order->req.contract->ticker_id, order->req.direction,
                                  order->req.offset, trade->volume, trade->price);
  } else if (trade->trade_type == TradeType::kAcquireStock) {
    auto contract = ContractTable::get_by_index(trade->ticker_id);
    assert(contract);
    pos_calculator_->UpdateComponentStock(contract->ticker_id, trade->volume, true);
  } else if (trade->trade_type == TradeType::kReleaseStock) {
    auto contract = ContractTable::get_by_index(trade->ticker_id);
    assert(contract);
    pos_calculator_->UpdateComponentStock(contract->ticker_id, trade->volume, false);
  }
}

void PositionManager::OnOrderCanceled(const Order* order, int canceled) {
  pos_calculator_->UpdatePending(order->req.contract->ticker_id, order->req.direction,
                                 order->req.offset, 0 - canceled);
}

void PositionManager::OnOrderRejected(const Order* order, int error_code) {
  if (error_code <= ERR_SEND_FAILED) return;

  pos_calculator_->UpdatePending(order->req.contract->ticker_id, order->req.direction,
                                 order->req.offset, 0 - order->req.volume);
}

}  // namespace ft
