// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_INCLUDE_CORE_POSITION_H_
#define FT_INCLUDE_CORE_POSITION_H_

#include <fmt/format.h>

#include <cstdint>
#include <string>

#include "cep/data/contract_table.h"

namespace ft {

struct PositionDetail {
  int yd_holdings = 0;
  int holdings = 0;
  int frozen = 0;
  int open_pending = 0;
  int close_pending = 0;

  double cost_price = 0;
  double float_pnl = 0;
};

struct Position {
  uint32_t tid = 0;
  PositionDetail long_pos;
  PositionDetail short_pos;
};

inline std::string dump_position(const Position& pos) {
  std::string_view ticker = "";
  auto contract = ContractTable::get_by_index(pos.tid);
  if (contract) ticker = contract->ticker;

  auto& lp = pos.long_pos;
  auto& sp = pos.short_pos;
  return fmt::format(
      "<Position ticker:{} |LONG holdings:{} yd_holdings:{} frozen:{} "
      "open_pending:{} close_pending:{} cost_price:{} |SHORT holdings:{} "
      "yd_holdings:{} frozen:{} open_pending:{} close_pending:{} "
      "cost_price:{}>",
      ticker, lp.holdings, lp.yd_holdings, lp.frozen, lp.open_pending,
      lp.close_pending, lp.cost_price, sp.holdings, sp.yd_holdings, sp.frozen,
      sp.open_pending, sp.close_pending, sp.cost_price);
}

}  // namespace ft

#endif  // FT_INCLUDE_CORE_POSITION_H_
