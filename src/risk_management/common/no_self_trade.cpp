// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "risk_management/common/no_self_trade.h"

#include "core/contract_table.h"
#include "utils/misc.h"

namespace ft {

bool NoSelfTradeRule::init(const Config& config, Account* account,
                           Portfolio* portfolio,
                           std::map<uint64_t, Order>* order_map,
                           const MdSnapshot* md_snapshot) {
  order_map_ = order_map;
  return true;
}

int NoSelfTradeRule::check_order_req(const Order* order) {
  if (order->req.direction != Direction::BUY ||
      order->req.direction != Direction::SELL)
    return NO_ERROR;

  auto* req = &order->req;
  const auto* contract = ContractTable::get_by_index(req->ticker_index);
  assert(contract);

  uint64_t opp_d = opp_direction(req->direction);  // 对手方
  const OrderReq* pending_order;
  for (auto& [engine_order_id, o] : *order_map_) {
    UNUSED(engine_order_id);
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

}  // namespace ft
