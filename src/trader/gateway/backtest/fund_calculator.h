// Copyright [2020-present] <Copyright Kevin, kevin.lau.gd@gmail.com>

#pragma once

#include "ft/base/market_data.h"
#include "ft/base/trade_msg.h"
#include "trader/msg.h"

namespace ft {

class FundCalculator {
 public:
  bool Init(const Account& init_fund);

  bool CheckFund(const OrderRequest& order) const;

  void UpdatePending(const OrderRequest& order, int changed);

  void UpdateTraded(const OrderRequest& order, int volume, double price,
                    const TickData& current_tick);

  void UpdatePrice(const Position& pos, const TickData& old_tick, const TickData& new_tick);

  const Account& GetFundAccount() const { return account_; }

 private:
  double GetMarketQuote(const TickData& tick) {
    if (tick.last_price > 0.0) {
      return tick.last_price;
    } else if (tick.ask[0] > 0.0) {
      if (tick.bid[0] > 0.0) {
        return (tick.ask[0] + tick.bid[0]) / 2;
      } else {
        return tick.ask[0];
      }
    } else {
      return tick.bid[0];
    }
  }

 private:
  Account account_{};
};

}  // namespace ft
