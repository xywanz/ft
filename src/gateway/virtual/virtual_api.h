// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_SRC_GATEWAY_VIRTUAL_VIRTUAL_API_H_
#define FT_SRC_GATEWAY_VIRTUAL_VIRTUAL_API_H_

#include <atomic>
#include <condition_variable>
#include <list>
#include <mutex>
#include <unordered_map>

#include "cep/data/constants.h"

namespace ft {

struct VirtualOrderReq {
  uint64_t oms_order_id;
  uint32_t ticker_index;
  uint32_t type;
  uint32_t direction;
  uint32_t offset;
  int volume;
  double price;

  // used internally
  bool to_canceled;
};

class VirtualGateway;

class VirtualApi {
 public:
  VirtualApi();

  void set_spi(VirtualGateway* gateway);

  void start_trade_server();

  void start_quote_server();

  bool insert_order(VirtualOrderReq* req);

  bool cancel_order(uint64_t order_id);

  void update_quote(uint32_t ticker_index, double ask, double bid);

 private:
  void process_pendings();

  void disseminate_market_data();

 private:
  struct LatestQuote {
    double bid = 0;
    double ask = 0;
  };

 private:
  VirtualGateway* gateway_;
  std::mutex mutex_;
  std::condition_variable cv_;
  std::unordered_map<uint32_t, LatestQuote> lastest_quotes_;
  std::list<VirtualOrderReq> pendings_;
  std::unordered_map<uint64_t, std::list<VirtualOrderReq>> limit_orders_;
};

}  // namespace ft

#endif  // FT_SRC_GATEWAY_VIRTUAL_VIRTUAL_API_H_
