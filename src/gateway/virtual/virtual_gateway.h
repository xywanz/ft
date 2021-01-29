// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_SRC_GATEWAY_VIRTUAL_VIRTUAL_GATEWAY_H_
#define FT_SRC_GATEWAY_VIRTUAL_VIRTUAL_GATEWAY_H_

#include <codecvt>
#include <limits>
#include <locale>
#include <map>
#include <string>
#include <vector>

#include "gateway/gateway.h"
#include "gateway/virtual/virtual_api.h"
#include "trading_server/datastruct/account.h"
#include "trading_server/datastruct/constants.h"
#include "trading_server/datastruct/contract.h"
#include "trading_server/datastruct/position.h"
#include "utils/contract_table.h"

namespace ft {

#define VIRTUAL_GATEWAY_VERSION "0.0.2"

class VirtualGateway : public Gateway {
 public:
  VirtualGateway();

  bool Login(BaseOrderManagementSystem* oms, const Config& config) override;
  void Logout() override;

  bool SendOrder(const OrderRequest& order, uint64_t* privdata_ptr) override;
  bool CancelOrder(uint64_t oms_order_id, uint64_t privdata) override;
  bool QueryContractList(std::vector<Contract>* result) override;

  bool QueryPositionList(std::vector<Position>* result) override;
  bool QueryAccount(Account* result) override;
  bool QueryTradeList(std::vector<Trade>* result) override;
  bool QueryMarginRate(const std::string& ticker) override;
  bool QueryCommisionRate(const std::string& ticker) override;

  void OnOrderAccepted(uint64_t oms_order_id);
  void OnOrderTraded(uint64_t oms_order_id, int traded, double price);
  void OnOrderCanceled(uint64_t oms_order_id, int canceled);

  void OnTick(TickData* tick);

 private:
  BaseOrderManagementSystem* oms_;
  VirtualApi virtual_api_;
};

}  // namespace ft

#endif  // FT_SRC_GATEWAY_VIRTUAL_VIRTUAL_GATEWAY_H_
