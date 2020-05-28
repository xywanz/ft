// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "risk_management/position_manager.h"

namespace ft {

PositionManager::PositionManager(Portfolio* portfolio)
    : portfolio_(portfolio) {}

int PositionManager::check_order_req(const Order* order) {
  auto* req = &order->req;
  if (is_offset_close(req->offset)) {
    int available = 0;
    auto pos =
        const_cast<const Portfolio*>(portfolio_)->find(req->ticker_index);

    if (pos) {
      uint32_t d = opp_direction(req->direction);
      auto& detail = d == Direction::BUY ? pos->long_pos : pos->short_pos;
      available = detail.holdings - detail.close_pending;
    }

    if (available < req->volume) {
      spdlog::error(
          "[PositionManager::check_order_req] Not enough volume to close. "
          "Available: {}, OrderVolume: {}",
          available, req->volume);
      return ERR_POSITION_NOT_ENOUGH;
    }
  }

  return NO_ERROR;
}

void PositionManager::on_order_sent(const Order* order) {
  portfolio_->update_pending(order->contract->index, order->req.direction,
                             order->req.offset, order->req.volume);
}

void PositionManager::on_order_traded(const Order* order, int this_traded,
                                      double traded_price) {
  portfolio_->update_traded(order->contract->index, order->req.direction,
                            order->req.offset, this_traded, traded_price);
}

void PositionManager::on_order_canceled(const Order* order, int canceled) {
  portfolio_->update_pending(order->contract->index, order->req.direction,
                             order->req.offset, 0 - canceled);
}

void PositionManager::on_order_rejected(const Order* order, int error_code) {
  if (error_code <= ERR_SEND_FAILED) return;

  portfolio_->update_pending(order->contract->index, order->req.direction,
                             order->req.offset, 0 - order->req.volume);
}

}  // namespace ft
