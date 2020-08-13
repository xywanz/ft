// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_SRC_GATEWAY_VIRTUAL_VIRTUAL_API_H_
#define FT_SRC_GATEWAY_VIRTUAL_VIRTUAL_API_H_

#include <atomic>
#include <condition_variable>
#include <list>
#include <mutex>
#include <unordered_map>

#include "cep/data/account.h"
#include "cep/data/constants.h"
#include "cep/oms/portfolio.h"

namespace ft {

struct VirtualOrderRequest {
  uint64_t oms_order_id;
  uint32_t tid;
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
 private:
  struct LatestQuote {
    double bid = 0;
    double ask = 0;
  };

 public:
  VirtualApi();
  void set_spi(VirtualGateway* gateway);
  void start_trade_server();
  void start_quote_server();

  bool insert_order(VirtualOrderRequest* req);
  bool cancel_order(uint64_t order_id);
  void update_quote(uint32_t tid, double ask, double bid);

  bool query_account(Account* result);

 private:
  void process_pendings();
  void disseminate_market_data();

  bool check_order(const VirtualOrderRequest* req) const;
  bool check_and_update_pos_account(const VirtualOrderRequest* req);
  void update_canceled(const VirtualOrderRequest& order);
  void update_traded(const VirtualOrderRequest& order,
                     const LatestQuote& quote);

 private:
  VirtualGateway* gateway_;
  std::mutex mutex_;
  std::condition_variable cv_;
  std::unordered_map<uint32_t, LatestQuote> lastest_quotes_;
  std::list<VirtualOrderRequest> pendings_;
  std::unordered_map<uint64_t, std::list<VirtualOrderRequest>> limit_orders_;
  Account account_{};
  Portfolio positions_;
};

}  // namespace ft

#endif  // FT_SRC_GATEWAY_VIRTUAL_VIRTUAL_API_H_
