// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "risk_management/futures/position_manager.h"

#include "core/contract_table.h"

namespace ft {

bool PositionManager::init(const Config& config, Account* account,
                           Portfolio* portfolio,
                           std::map<uint64_t, Order>* order_map,
                           const MdSnapshot* md_snapshot) {
  portfolio_ = portfolio;
  return true;
}

int PositionManager::check_order_req(const Order* order) {
  if (order->req.direction != Direction::BUY ||
      order->req.direction != Direction::SELL)
    return NO_ERROR;

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

void PositionManager::on_order_traded(const Order* order,
                                      const OrderTradedRsp* trade) {
  if (trade->trade_type == TradeType::SECONDARY_MARKET ||
      trade->trade_type == TradeType::PRIMARY_MARKET) {
    portfolio_->update_traded(order->req.ticker_index, order->req.direction,
                              order->req.offset, trade->volume, trade->price);
  } else if (trade->trade_type == TradeType::ACQUIRED_STOCK) {
    auto contract = ContractTable::get_by_index(trade->ticker_index);
    assert(contract);
    portfolio_->update_traded(contract->index, Direction::BUY, Offset::OPEN,
                              trade->volume, trade->price, false);
  } else if (trade->trade_type == TradeType::RELEASED_STOCK) {
    auto contract = ContractTable::get_by_index(trade->ticker_index);
    assert(contract);
    portfolio_->update_traded(contract->index, Direction::SELL,
                              Offset::CLOSE_YESTERDAY, trade->volume,
                              trade->price, false);
  }
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
