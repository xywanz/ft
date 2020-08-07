// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_SRC_GATEWAY_CTP_CTP_GATEWAY_H_
#define FT_SRC_GATEWAY_CTP_CTP_GATEWAY_H_

#include <ThostFtdcMdApi.h>
#include <ThostFtdcTraderApi.h>

#include <memory>
#include <string>
#include <vector>

#include "cep/interface/gateway.h"
#include "ctp_common.h"
#include "ctp_quote_api.h"
#include "ctp_trade_api.h"

namespace ft {

class CtpGateway : public Gateway {
 public:
  CtpGateway();
  ~CtpGateway();

  bool login(OMSInterface *oms, const Config &config) override;
  void logout() override;

  bool send_order(const OrderRequest &order) override;
  bool cancel_order(uint64_t order_id) override;

  bool subscribe(const std::vector<std::string> &sub_list) override;

  bool query_contracts(std::vector<Contract> *result) override;
  bool query_positions(std::vector<Position> *result) override;
  bool query_account(Account *result) override;
  bool query_trades(std::vector<Trade> *result) override;
  bool query_margin_rate(const std::string &ticker) override;

 private:
  std::unique_ptr<CtpTradeApi> trade_api_;
  std::unique_ptr<CtpQuoteApi> quote_api_;

  // trade

  // md
};

}  // namespace ft

#endif  // FT_SRC_GATEWAY_CTP_CTP_GATEWAY_H_
