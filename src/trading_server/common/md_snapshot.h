// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_SRC_TRADING_SERVER_COMMON_MD_SNAPSHOT_H_
#define FT_SRC_TRADING_SERVER_COMMON_MD_SNAPSHOT_H_

#include <map>
#include <string>
#include <vector>

#include "protocol/data_types.h"
#include "utils/contract_table.h"

namespace ft {

struct TickDataSnapshot {
  double last_price;
  double ask;
  double bid;
  double iopv;

  double upper_limit_price;
  double lower_limit_price;
};

class MarketDataSnashot {
 public:
  MarketDataSnashot() {}

  void Init() { snapshot_.resize(ContractTable::size() + 1, nullptr); }

  const TickDataSnapshot* get(uint32_t ticker_id) const { return snapshot_[ticker_id]; }

  void UpdateSnapshot(const TickData& tick) {
    auto data = snapshot_[tick.ticker_id];
    if (data) {
      data->last_price = tick.last_price;
      data->bid = tick.bid[0];
      data->ask = tick.ask[0];
      data->iopv = tick.etf.iopv;
    } else {
      data = new TickDataSnapshot;
      data->last_price = tick.last_price;
      data->bid = tick.bid[0];
      data->ask = tick.ask[0];
      data->iopv = tick.etf.iopv;
      data->upper_limit_price = tick.upper_limit_price;
      data->lower_limit_price = tick.lower_limit_price;
      snapshot_[tick.ticker_id] = data;
    }
  }

 private:
  std::vector<TickDataSnapshot*> snapshot_;
};

}  // namespace ft

#endif  // FT_SRC_TRADING_SERVER_COMMON_MD_SNAPSHOT_H_
