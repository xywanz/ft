// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "trading_server/risk_management/common/position_manager.h"

#include <spdlog/spdlog.h>

#include "trading_server/datastruct/contract_table.h"

namespace ft {

bool PositionManager::Init(RiskRuleParams* params) {
  portfolio_ = params->portfolio;
  return true;
}

int PositionManager::CheckOrderRequest(const Order* order) {
  if (order->req.direction != Direction::BUY && order->req.direction != Direction::SELL)
    return NO_ERROR;

  auto* req = &order->req;
  if (IsOffsetClose(req->offset)) {
    int available = 0;
    auto pos = const_cast<const Portfolio*>(portfolio_)->get_position(req->contract->ticker_id);

    if (pos) {
      uint32_t d = OppositeDirection(req->direction);
      auto& detail = d == Direction::BUY ? pos->long_pos : pos->short_pos;
      available = detail.holdings - detail.close_pending;
    }

    if (available < req->volume) {
      spdlog::error(
          "[PositionManager::CheckOrderRequest] Not enough volume to Close. "
          "Available: {}, OrderVolume: {}",
          available, req->volume);
      return ERR_POSITION_NOT_ENOUGH;
    }
  }

  return NO_ERROR;
}

void PositionManager::OnOrderSent(const Order* order) {
  portfolio_->UpdatePending(order->req.contract->ticker_id, order->req.direction, order->req.offset,
                            order->req.volume);
}

void PositionManager::OnOrderTraded(const Order* order, const Trade* trade) {
  if (trade->trade_type == TradeType::SECONDARY_MARKET ||
      trade->trade_type == TradeType::PRIMARY_MARKET) {
    portfolio_->UpdateTraded(order->req.contract->ticker_id, order->req.direction,
                             order->req.offset, trade->volume, trade->price);
  } else if (trade->trade_type == TradeType::ACQUIRED_STOCK) {
    auto contract = ContractTable::get_by_index(trade->ticker_id);
    assert(contract);
    portfolio_->UpdateComponentStock(contract->ticker_id, trade->volume, true);
  } else if (trade->trade_type == TradeType::RELEASED_STOCK) {
    auto contract = ContractTable::get_by_index(trade->ticker_id);
    assert(contract);
    portfolio_->UpdateComponentStock(contract->ticker_id, trade->volume, false);
  }
}

void PositionManager::OnOrderCanceled(const Order* order, int canceled) {
  portfolio_->UpdatePending(order->req.contract->ticker_id, order->req.direction, order->req.offset,
                            0 - canceled);
}

void PositionManager::OnOrderRejected(const Order* order, int error_code) {
  if (error_code <= ERR_SEND_FAILED) return;

  portfolio_->UpdatePending(order->req.contract->ticker_id, order->req.direction, order->req.offset,
                            0 - order->req.volume);
}

}  // namespace ft
