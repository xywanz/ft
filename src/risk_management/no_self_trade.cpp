// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "risk_management/no_self_trade.h"

#include "core/contract_table.h"

namespace ft {

int NoSelfTradeRule::check_order_req(const Order* order) {
  auto* req = &order->req;
  const auto* contract = ContractTable::get_by_index(req->ticker_index);
  assert(contract);

  uint64_t opp_d = opp_direction(req->direction);  // 对手方
  const OrderReq* pending_order;
  for (auto& o : orders_) {
    pending_order = &o.req;
    if (pending_order->direction != opp_d) continue;

    // 存在市价单直接拒绝
    if (pending_order->type == OrderType::MARKET) goto catch_order;

    if (req->direction == Direction::BUY) {
      if (req->price > pending_order->price - 1e-5) goto catch_order;
    } else {
      if (req->price < pending_order->price + 1e-5) goto catch_order;
    }
  }

  orders_.emplace_back(*order);
  return NO_ERROR;

catch_order:
  spdlog::error(
      "[RiskMgr] Self trade! Ticker: {}. This Order: "
      "[Direction: {}, Type: {}, Price: {:.2f}]. "
      "Pending Order: [Direction: {}, Type: {}, Price: {:.2f}]",
      contract->ticker, direction_str(req->direction), ordertype_str(req->type),
      req->price, direction_str(pending_order->direction),
      ordertype_str(pending_order->type), pending_order->price);
  return ERR_SELF_TRADE;
}

void NoSelfTradeRule::on_order_completed(const Order* order) {
  for (auto iter = orders_.begin(); iter != orders_.end(); ++iter) {
    if (iter->engine_order_id == order->engine_order_id) {
      orders_.erase(iter);
      return;
    }
  }
}

}  // namespace ft
