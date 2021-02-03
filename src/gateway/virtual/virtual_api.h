// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_SRC_GATEWAY_VIRTUAL_VIRTUAL_API_H_
#define FT_SRC_GATEWAY_VIRTUAL_VIRTUAL_API_H_

#include <atomic>
#include <condition_variable>
#include <list>
#include <mutex>
#include <unordered_map>

#include "trading_server/datastruct/account.h"
#include "trading_server/datastruct/constants.h"
#include "utils/portfolio.h"

namespace ft {

struct VirtualOrderRequest {
  uint64_t oms_order_id;
  uint32_t ticker_id;
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
  void StartTradeServer();
  void StartQuoteServer();

  bool InsertOrder(VirtualOrderRequest* req);
  bool CancelOrder(uint64_t order_id);
  void UpdateQuote(uint32_t ticker_id, double ask, double bid);

  bool QueryAccount(Account* result);

 private:
  void process_pendings();
  void DisseminateMarketData();

  bool CheckOrder(const VirtualOrderRequest* req) const;
  bool CheckAndUpdatePosAccount(const VirtualOrderRequest* req);
  void UpdateCanceled(const VirtualOrderRequest& order);
  void UpdateTraded(const VirtualOrderRequest& order, const LatestQuote& quote);

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
