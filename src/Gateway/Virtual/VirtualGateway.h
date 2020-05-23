// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_SRC_GATEWAY_VIRTUAL_VIRTUALGATEWAY_H_
#define FT_SRC_GATEWAY_VIRTUAL_VIRTUALGATEWAY_H_

#include <codecvt>
#include <limits>
#include <locale>
#include <map>
#include <string>
#include <vector>

#include "Core/Account.h"
#include "Core/Constants.h"
#include "Core/Contract.h"
#include "Core/ContractTable.h"
#include "Core/Gateway.h"
#include "Core/Position.h"
#include "Gateway/Virtual/VirtualTradeApi.h"

namespace ft {

class VirtualGateway : public Gateway {
 public:
  explicit VirtualGateway(TradingEngineInterface* engine);

  bool login(const LoginParams& params) override;

  void logout() override;

  uint64_t send_order(const OrderReq* order) override;

  bool cancel_order(uint64_t order_id) override;

  bool query_contract(const std::string& ticker,
                      const std::string& exchange) override;

  bool query_contracts() override;

  bool query_position(const std::string& ticker) override;

  bool query_positions() override;

  bool query_account() override;

  bool query_trades() override;

  bool query_margin_rate(const std::string& ticker) override;

  bool query_commision_rate(const std::string& ticker) override;

  void on_order_accepted(uint64_t order_id);

  void on_order_traded(uint64_t order_id, int traded, double price);

  void on_order_canceled(uint64_t order_id, int canceled);

 private:
  TradingEngineInterface* engine_;
  VirtualTradeApi trade_api_;
};

}  // namespace ft

#endif  // FT_SRC_GATEWAY_VIRTUAL_VIRTUALGATEWAY_H_
