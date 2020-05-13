// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_GATEWAY_CTP_CTPGATEWAY_H_
#define FT_GATEWAY_CTP_CTPGATEWAY_H_

#include <ThostFtdcMdApi.h>
#include <ThostFtdcTraderApi.h>

#include <atomic>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "Core/Gateway.h"
#include "Ctp/CtpCommon.h"
#include "Ctp/CtpMdApi.h"
#include "Ctp/CtpTradeApi.h"

namespace ft {

class CtpGateway : public Gateway {
 public:
  explicit CtpGateway(TradingEngineInterface *engine);

  ~CtpGateway();

  bool login(const LoginParams &params);

  void logout();

  uint64_t send_order(const OrderReq *order);

  bool cancel_order(uint64_t order_id);

  bool query_contract(const std::string &ticker) override;

  bool query_contracts() override;

  bool query_position(const std::string &ticker);

  bool query_positions() override;

  bool query_account() override;

  bool query_orders();

  bool query_trades();

  bool query_margin_rate(const std::string &ticker) override;

 private:
  std::unique_ptr<CtpTradeApi> trade_api_;
  std::unique_ptr<CtpMdApi> md_api_;

  // trade

  // md
};

}  // namespace ft

#endif  // FT_GATEWAY_CTP_CTPGATEWAY_H_
