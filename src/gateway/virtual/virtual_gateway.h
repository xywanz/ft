// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_SRC_GATEWAY_VIRTUAL_VIRTUAL_GATEWAY_H_
#define FT_SRC_GATEWAY_VIRTUAL_VIRTUAL_GATEWAY_H_

#include <codecvt>
#include <limits>
#include <locale>
#include <map>
#include <string>
#include <vector>

#include "core/account.h"
#include "core/constants.h"
#include "core/contract.h"
#include "core/contract_table.h"
#include "core/gateway.h"
#include "core/position.h"
#include "gateway/virtual/virtual_api.h"

namespace ft {

#define VIRTUAL_GATEWAY_VERSION "0.0.2"

class VirtualGateway : public Gateway {
 public:
  VirtualGateway();

  bool login(TradingEngineInterface* engine, const Config& config) override;

  void logout() override;

  bool send_order(const OrderReq* order) override;

  bool cancel_order(uint64_t engine_order_id) override;

  bool query_contract(const std::string& ticker,
                      const std::string& exchange) override;

  bool query_contracts() override;

  bool query_position(const std::string& ticker) override;

  bool query_positions() override;

  bool query_account() override;

  bool query_trades() override;

  bool query_margin_rate(const std::string& ticker) override;

  bool query_commision_rate(const std::string& ticker) override;

  void on_order_accepted(uint64_t engine_order_id);

  void on_order_traded(uint64_t engine_order_id, int traded, double price);

  void on_order_canceled(uint64_t engine_order_id, int canceled);

  void on_tick(const TickData* tick);

 private:
  TradingEngineInterface* engine_;
  VirtualApi virtual_api_;
};

}  // namespace ft

#endif  // FT_SRC_GATEWAY_VIRTUAL_VIRTUAL_GATEWAY_H_
