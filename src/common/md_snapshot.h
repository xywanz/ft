// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_SRC_COMMON_MD_SNAPSHOT_H_
#define FT_SRC_COMMON_MD_SNAPSHOT_H_

#include <map>
#include <string>
#include <vector>

#include "core/contract_table.h"
#include "core/tick_data.h"

namespace ft {

struct TickDataSnapshot {
  double last_price;
  double ask;
  double bid;
  double iopv;
};

class MdSnapshot {
 public:
  MdSnapshot() { snapshot_.resize(ContractTable::size() + 1, nullptr); }

  const TickDataSnapshot* get(uint32_t ticker_index) const {
    return snapshot_[ticker_index];
  }

  void update_snapshot(const TickData& tick) {
    auto data = snapshot_[tick.ticker_index];
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
      snapshot_[tick.ticker_index] = data;
    }
  }

 private:
  std::vector<TickDataSnapshot*> snapshot_;
};

}  // namespace ft

#endif  // FT_SRC_COMMON_MD_SNAPSHOT_H_
