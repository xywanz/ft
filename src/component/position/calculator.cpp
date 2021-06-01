// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "ft/component/position/calculator.h"

#include "ft/base/contract_table.h"
#include "ft/base/log.h"
#include "ft/utils/protocol_utils.h"

namespace ft {

PositionCalculator::PositionCalculator() {}

void PositionCalculator::SetCallback(CallbackType&& f) { cb_ = std::move(f); }

bool PositionCalculator::SetPosition(const Position& pos) {
  auto* contract = ContractTable::get_by_index(pos.ticker_id);
  if (!contract) {
    LOG_ERROR("[PositionCalculator::SetPosition] contract not found. ticker_id:{}", pos.ticker_id);
    return false;
  }

  positions_[pos.ticker_id] = pos;
  if (cb_) {
    cb_(pos);
  }
  return true;
}

bool PositionCalculator::UpdatePending(uint32_t ticker_id, Direction direction, Offset offset,
                                       int changed) {
  if (changed == 0) {
    return true;
  }

  bool is_close = IsOffsetClose(offset);
  if (is_close) {
    direction = OppositeDirection(direction);
  }

  auto& pos = positions_[ticker_id];
  pos.ticker_id = ticker_id;
  auto& pos_detail = direction == Direction::kBuy ? pos.long_pos : pos.short_pos;
  if (is_close)
    pos_detail.close_pending += changed;
  else
    pos_detail.open_pending += changed;

  assert(pos_detail.open_pending >= 0);
  assert(pos_detail.close_pending >= 0);

  if (cb_) {
    cb_(pos);
  }
  return true;
}

bool PositionCalculator::UpdateTraded(uint32_t ticker_id, Direction direction, Offset offset,
                                      int traded, double traded_price) {
  if (traded <= 0) {
    return true;
  }

  const auto* contract = ContractTable::get_by_index(ticker_id);
  if (!contract) {
    LOG_ERROR("[PositionCalculator::UpdateTraded] contract not found. ticker_id:{}", ticker_id);
    return false;
  }
  assert(contract->size > 0);

  bool is_close = IsOffsetClose(offset);
  if (is_close) {
    direction = OppositeDirection(direction);
  }

  auto& pos = positions_[ticker_id];
  pos.ticker_id = ticker_id;
  auto& pos_detail = direction == Direction::kBuy ? pos.long_pos : pos.short_pos;

  if (is_close) {
    pos_detail.close_pending -= traded;
    pos_detail.holdings -= traded;
    // 为了防止有些交易所不区分昨今仓，但用户平仓的时候却使用了close_yesterday
    if (offset == Offset::kCloseYesterday || offset == Offset::kClose)
      pos_detail.yd_holdings -= std::min(pos_detail.yd_holdings, traded);

    if (pos_detail.holdings < pos_detail.yd_holdings) {
      pos_detail.yd_holdings = pos_detail.holdings;
    }
  } else {
    pos_detail.open_pending -= traded;
    pos_detail.holdings += traded;
  }

  assert(pos_detail.holdings >= 0);
  assert(pos_detail.open_pending >= 0);
  assert(pos_detail.close_pending >= 0);

  // 如果是开仓则计算当前持仓的成本价
  if (IsOffsetOpen(offset) && pos_detail.holdings > 0) {
    double cost = contract->size * (pos_detail.holdings - traded) * pos_detail.cost_price +
                  contract->size * traded * traded_price;
    pos_detail.cost_price = cost / (pos_detail.holdings * contract->size);
  }

  if (pos_detail.holdings == 0) {
    pos_detail.cost_price = 0.0;
    pos_detail.float_pnl = 0.0;
  }

  if (cb_) {
    cb_(pos);
  }
  return true;
}

bool PositionCalculator::ModifyPostition(uint32_t ticker_id, Direction direction, int changed) {
  if (changed == 0) {
    return true;
  }

  auto& pos = positions_[ticker_id];
  pos.ticker_id = ticker_id;

  auto& pos_detail = direction == Direction::kBuy ? pos.long_pos : pos.short_pos;
  if (changed < 0 && pos_detail.holdings - pos_detail.close_pending + changed < 0) {
    LOG_ERROR("PositionCalculator::ModifyPostition. position not enough. {} {} {}", ticker_id,
              ToString(direction), changed);
    return false;
  }
  pos_detail.holdings += changed;

  if (cb_) {
    cb_(pos);
  }
  return true;
}

bool PositionCalculator::UpdateFloatPnl(uint32_t ticker_id, double bid, double ask) {
  auto it = positions_.find(ticker_id);
  if (it == positions_.end()) {
    return true;
  }
  auto& pos = it->second;
  pos.ticker_id = ticker_id;
  if (pos.long_pos.holdings > 0 || pos.short_pos.holdings > 0) {
    const auto* contract = ContractTable::get_by_index(ticker_id);
    if (!contract || contract->size <= 0) {
      LOG_ERROR("[PositionCalculator::UpdateFloatPnl] invalid contract");
      return false;
    }

    auto& lp = pos.long_pos;
    auto& sp = pos.short_pos;

    if (lp.holdings > 0 && ask > 0.0) {
      lp.float_pnl = lp.holdings * contract->size * (ask - lp.cost_price);
    }

    if (sp.holdings > 0 && bid > 0.0) {
      sp.float_pnl = sp.holdings * contract->size * (sp.cost_price - bid);
    }

    if (cb_ && (lp.holdings > 0 || sp.holdings > 0)) {
      cb_(pos);
    }
  }
  return true;
}

double PositionCalculator::TotalAssets() const {
  double value = 0.0;

  for (auto& [ticker_id, pos] : positions_) {
    const auto* contract = ContractTable::get_by_index(ticker_id);
    if (!contract || contract->size <= 0) {
      throw std::runtime_error("contract not found");
    }

    auto& lp = pos.long_pos;
    auto& sp = pos.short_pos;

    if (lp.holdings > 0) {
      value += lp.holdings * contract->size * lp.cost_price + lp.float_pnl;
    }

    if (sp.holdings > 0) {
      value += sp.holdings * contract->size * sp.cost_price + sp.float_pnl;
    }
  }

  return value;
}

}  // namespace ft
