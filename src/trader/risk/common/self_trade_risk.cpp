// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "trader/risk/common/self_trade_risk.h"

#include "ft/base/contract_table.h"
#include "ft/base/log.h"
#include "ft/utils/misc.h"
#include "ft/utils/protocol_utils.h"

namespace ft {

bool SelfTradeRisk::Init(RiskRuleParams* params) {
  order_map_ = params->order_map;
  LOG_INFO("self trade risk inited");
  return true;
}

ErrorCode SelfTradeRisk::CheckOrderRequest(const Order& order) {
  auto& req = order.req;
  auto contract = req.contract;

  auto oppsite_direction = OppositeDirection(req.direction);  // 对手方
  const OrderRequest* pending_order;
  for (auto& [oms_order_id, o] : *order_map_) {
    UNUSED(oms_order_id);
    pending_order = &o.req;
    if (pending_order->contract->ticker_id != req.contract->ticker_id) {
      continue;
    }
    if (pending_order->direction != oppsite_direction) {
      continue;
    }

    // 存在市价单直接拒绝
    if (pending_order->price < 1e-5 || pending_order->type == OrderType::kMarket ||
        (req.direction == Direction::kBuy && req.price > pending_order->price - 1e-5) ||
        (req.direction == Direction::kSell && req.price < pending_order->price + 1e-5)) {
      LOG_ERROR(
          "[RiskMgr] Self trade! Ticker: {}. This Order: [Direction: {}, Type: {}, Price: {:.2f}]. "
          "Pending Order: [Direction: {}, Type: {}, Price: {:.2f}]",
          contract->ticker, ToString(req.direction), ToString(req.type), req.price,
          ToString(pending_order->direction), ToString(pending_order->type), pending_order->price);
      return ErrorCode::kSelfTrade;
    }
  }

  return ErrorCode::kNoError;
}

REGISTER_RISK_RULE("ft.risk.self_trade", SelfTradeRisk);

}  // namespace ft
