// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "ft/component/position_calculator.h"

#include "ft/base/contract_table.h"
#include "ft/utils/protocol_utils.h"

namespace ft {

PositionCalculator::PositionCalculator() {}

void PositionCalculator::SetCallback(std::function<void(const Position&)>&& f) {
  cb_ = std::make_unique<CallbackType>(std::move(f));
}

void PositionCalculator::SetPosition(const Position& pos) {
  positions_[pos.ticker_id] = pos;

  if (cb_) {
    (*cb_)(pos);
  }
}

void PositionCalculator::UpdatePending(uint32_t ticker_id, Direction direction, Offset offset,
                                       int changed) {
  if (changed == 0) {
    return;
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
    (*cb_)(pos);
  }
}

void PositionCalculator::UpdateTraded(uint32_t ticker_id, Direction direction, Offset offset,
                                      int traded, double traded_price) {
  if (traded <= 0) {
    return;
  }

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

  const auto* contract = ContractTable::get_by_index(ticker_id);
  if (!contract) {
    throw std::runtime_error("contract not found");
  }
  assert(contract->size > 0);

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
    (*cb_)(pos);
  }
}

void PositionCalculator::UpdateFloatPnl(uint32_t ticker_id, double bid, double ask) {
  auto& pos = positions_[ticker_id];
  pos.ticker_id = ticker_id;
  if (pos.long_pos.holdings > 0 || pos.short_pos.holdings > 0) {
    const auto* contract = ContractTable::get_by_index(ticker_id);
    if (!contract || contract->size <= 0) {
      return;
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
      (*cb_)(pos);
    }
  }
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
