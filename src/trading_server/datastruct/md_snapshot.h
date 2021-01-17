// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_SRC_TRADING_SERVER_DATASTRUCT_MD_SNAPSHOT_H_
#define FT_SRC_TRADING_SERVER_DATASTRUCT_MD_SNAPSHOT_H_

#include <map>
#include <string>
#include <vector>

#include "trading_server/datastruct/contract_table.h"
#include "trading_server/datastruct/tick_data.h"

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

  const TickDataSnapshot* get(uint32_t tid) const { return snapshot_[tid]; }

  void UpdateSnapshot(const TickData& tick) {
    auto data = snapshot_[tick.tid];
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
      snapshot_[tick.tid] = data;
    }
  }

 private:
  std::vector<TickDataSnapshot*> snapshot_;
};

}  // namespace ft

#endif  // FT_SRC_TRADING_SERVER_DATASTRUCT_MD_SNAPSHOT_H_
