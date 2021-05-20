// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_SRC_GATEWAY_CTP_CTP_GATEWAY_H_
#define FT_SRC_GATEWAY_CTP_CTP_GATEWAY_H_

#include <memory>
#include <string>
#include <vector>

#include "ThostFtdcMdApi.h"
#include "ThostFtdcTraderApi.h"
#include "trader/gateway/ctp/ctp_common.h"
#include "trader/gateway/ctp/ctp_quote_api.h"
#include "trader/gateway/ctp/ctp_trade_api.h"
#include "trader/gateway/gateway.h"

namespace ft {

class CtpGateway : public Gateway {
 public:
  CtpGateway();
  ~CtpGateway();

  bool Init(const GatewayConfig &config) override;
  void Logout() override;

  bool SendOrder(const OrderRequest &order, uint64_t *privdata_ptr) override;
  bool CancelOrder(uint64_t order_id, uint64_t privdata) override;

  bool Subscribe(const std::vector<std::string> &sub_list) override;

  bool QueryContracts() override;
  bool QueryPositions() override;
  bool QueryAccount() override;
  bool QueryTrades() override;

 private:
  friend CtpTradeApi;
  friend CtpQuoteApi;

  std::unique_ptr<CtpTradeApi> trade_api_;
  std::unique_ptr<CtpQuoteApi> quote_api_;

  // trade

  // md
};

}  // namespace ft

#endif  // FT_SRC_GATEWAY_CTP_CTP_GATEWAY_H_
