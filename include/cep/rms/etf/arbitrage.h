// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_SRC_RISK_MANAGEMENT_ETF_ARBITRAGE_H_
#define FT_SRC_RISK_MANAGEMENT_ETF_ARBITRAGE_H_

#include <unordered_map>

namespace ft {

struct DemandDetail {
  uint32_t total_demand;

  uint32_t volume_to_use_holdings;
  uint32_t volume_to_trade;
  uint32_t current_traded;
};

class Arbitrage {
 public:
  explicit Arbitrage(uint32_t etf_tid);

 private:
  std::unordered_map<uint32_t, DemandDetail> demands_;
};

}  // namespace ft

#endif  // FT_SRC_RISK_MANAGEMENT_ETF_ARBITRAGE_H_
