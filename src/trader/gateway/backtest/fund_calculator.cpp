// Copyright [2020-present] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "trader/gateway/backtest/fund_calculator.h"

#include <cassert>

#include "ft/base/contract_table.h"
#include "ft/base/log.h"
#include "ft/utils/protocol_utils.h"

namespace ft {

bool FundCalculator::Init(const Account& init_fund) {
  account_ = init_fund;
  return true;
}

bool FundCalculator::CheckFund(const OrderRequest& order) const {
  if (IsOffsetOpen(order.offset)) {
    double fund_need = order.contract->size * order.volume * order.price;
    if (account_.cash < fund_need) {
      LOG_ERROR("fund not enough. cash:{} need:{}", account_.cash, fund_need);
      return false;
    }
  }

  return true;
}

void FundCalculator::UpdatePending(const OrderRequest& order, int changed) {
  if (IsOffsetOpen(order.offset)) {
    double fund_need = order.contract->size * changed * order.price;
    account_.cash -= fund_need;
    account_.frozen += fund_need;
  }
}

void FundCalculator::UpdateTraded(const OrderRequest& order, int volume, double price,
                                  const TickData& current_tick) {
  if (IsOffsetOpen(order.offset)) {
    double fund_return = order.contract->size * volume * order.price;
    account_.frozen -= fund_return;
    account_.cash += fund_return;

    account_.margin += order.contract->size * volume * GetMarketQuote(current_tick);
    account_.cash -= order.contract->size * volume * price;
  } else {
    // TODO(K): update balance and cash
    account_.margin -= order.contract->size * volume * GetMarketQuote(current_tick);
    account_.cash += order.contract->size * volume * price;
  }
}

void FundCalculator::UpdatePrice(const Position& pos, const TickData& old_tick,
                                 const TickData& new_tick) {
  auto* contract = ContractTable::get_by_index(pos.ticker_id);
  assert(contract);

  int long_pos = pos.long_pos.holdings;
  int short_pos = pos.short_pos.holdings;
  int total_pos = long_pos + short_pos;

  // update margin
  double margin_changed =
      contract->size * total_pos * (GetMarketQuote(new_tick) - GetMarketQuote(old_tick));
  account_.margin += margin_changed;
  account_.cash -= margin_changed;

  // TODO(K): update floating_pnl and cash
  // account_.cash = account_.balance + account_.floating_pnl;
}

}  // namespace ft
