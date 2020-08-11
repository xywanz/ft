// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_SRC_GATEWAY_XTP_XTP_GATEWAY_H_
#define FT_SRC_GATEWAY_XTP_XTP_GATEWAY_H_

#include <xtp_quote_api.h>
#include <xtp_trader_api.h>

#include <memory>
#include <string>
#include <vector>

#include "_xtp_quote_api.h"
#include "cep/interface/gateway.h"
#include "xtp_common.h"
#include "xtp_trade_api.h"

namespace ft {

class XtpGateway : public Gateway {
 public:
  XtpGateway();

  bool login(OMSInterface* oms, const Config& config) override;
  void logout() override;
  bool subscribe(const std::vector<std::string>& sub_list) override;

  bool send_order(const OrderRequest& order, uint64_t* privdata_ptr) override;
  bool cancel_order(uint64_t order_id, uint64_t privdata) override;

  bool query_contracts(std::vector<Contract>* result) override;
  bool query_account(Account* result) override;
  bool query_positions(std::vector<Position>* result) override;
  bool query_trades(std::vector<Trade>* result) override;
  bool query_orders();

 private:
  std::unique_ptr<XtpTradeApi> trade_api_;
  std::unique_ptr<XtpQuoteApi> quote_api_;
};

}  // namespace ft

#endif  // FT_SRC_GATEWAY_XTP_XTP_GATEWAY_H_
