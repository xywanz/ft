// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_SRC_GATEWAY_VIRTUAL_VIRTUAL_GATEWAY_H_
#define FT_SRC_GATEWAY_VIRTUAL_VIRTUAL_GATEWAY_H_

#include <codecvt>
#include <limits>
#include <locale>
#include <map>
#include <string>
#include <vector>

#include "cep/data/account.h"
#include "cep/data/constants.h"
#include "cep/data/contract.h"
#include "cep/data/contract_table.h"
#include "cep/data/position.h"
#include "cep/interface/gateway.h"
#include "virtual_api.h"

namespace ft {

#define VIRTUAL_GATEWAY_VERSION "0.0.2"

class VirtualGateway : public Gateway {
 public:
  VirtualGateway();

  bool login(OMSInterface* oms, const Config& config) override;
  void logout() override;

  bool send_order(const OrderRequest& order, uint64_t* privdata_ptr) override;
  bool cancel_order(uint64_t oms_order_id, uint64_t privdata) override;
  bool query_contracts(std::vector<Contract>* result) override;

  bool query_positions(std::vector<Position>* result) override;
  bool query_account(Account* result) override;
  bool query_trades(std::vector<Trade>* result) override;
  bool query_margin_rate(const std::string& ticker) override;
  bool query_commision_rate(const std::string& ticker) override;

  void on_order_accepted(uint64_t oms_order_id);
  void on_order_traded(uint64_t oms_order_id, int traded, double price);
  void on_order_canceled(uint64_t oms_order_id, int canceled);

  void on_tick(TickData* tick);

 private:
  OMSInterface* oms_;
  VirtualApi virtual_api_;
};

}  // namespace ft

#endif  // FT_SRC_GATEWAY_VIRTUAL_VIRTUAL_GATEWAY_H_
