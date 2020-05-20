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
  explicit VirtualGateway(TradingEngineInterface* engine) : Gateway(engine) {}

  bool login(const LoginParams& params) override;

  void logout() override;

  bool send_order(const OrderReq* order) override;

  bool cancel_order(uint64_t order_id) override;

  bool query_contract(const std::string& ticker) override;

  bool query_contracts() override;

  bool query_position(const std::string& ticker) override;

  bool query_positions() override;

  bool query_account() override;

  bool query_trades() override;

  bool query_margin_rate(const std::string& ticker) override;

  bool query_commision_rate(const std::string& ticker) override;

 private:
  VirtualTradeApi trade_api_;
};

}  // namespace ft

#endif  // FT_SRC_GATEWAY_VIRTUAL_VIRTUALGATEWAY_H_
