#ifndef FT_SRC_MARKET_MARKET_SERVER_H_
#define FT_SRC_MARKET_MARKET_SERVER_H_

#include "ft/base/config.h"

namespace ft {

class MarketServer {
 public:
  bool Init(const FlareTraderConfig& conf);

  void Run();

 private:
};

}  // namespace ft

#endif  // FT_SRC_MARKET_MARKET_SERVER_H_
