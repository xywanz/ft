// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_SRC_GATEWAY_CTP_CTP_GATEWAY_H_
#define FT_SRC_GATEWAY_CTP_CTP_GATEWAY_H_

#include <ThostFtdcMdApi.h>
#include <ThostFtdcTraderApi.h>

#include <memory>
#include <string>

#include "gateway/ctp/ctp_common.h"
#include "gateway/ctp/ctp_quote_api.h"
#include "gateway/ctp/ctp_trade_api.h"
#include "interface/gateway.h"

namespace ft {

class CtpGateway : public Gateway {
 public:
  CtpGateway();

  ~CtpGateway();

  bool login(TradingEngineInterface *engine, const Config &config) override;

  void logout() override;

  bool send_order(const OrderReq &order) override;

  bool cancel_order(uint64_t order_id) override;

  bool query_contract(const std::string &ticker,
                      const std::string &exchange) override;

  bool query_contracts() override;

  bool query_position(const std::string &ticker) override;

  bool query_positions() override;

  bool query_account() override;

  bool query_trades() override;

  bool query_margin_rate(const std::string &ticker) override;

 private:
  std::unique_ptr<CtpTradeApi> trade_api_;
  std::unique_ptr<CtpQuoteApi> quote_api_;

  // trade

  // md
};

}  // namespace ft

#endif  // FT_SRC_GATEWAY_CTP_CTP_GATEWAY_H_
