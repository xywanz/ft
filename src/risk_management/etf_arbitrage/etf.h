// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_SRC_RISK_MANAGEMENT_ETF_ARBITRAGE_ETF_H_
#define FT_SRC_RISK_MANAGEMENT_ETF_ARBITRAGE_ETF_H_

namespace ft {

enum ReplaceType {
  FORBIDDEN = 0,
  OPTIONAL = 1,
  MUST = 2,
  RECOMPUTE = 3,
};

struct ComponentStock {};

struct ETF {};

}  // namespace ft

#endif  // FT_SRC_RISK_MANAGEMENT_ETF_ARBITRAGE_ETF_H_
