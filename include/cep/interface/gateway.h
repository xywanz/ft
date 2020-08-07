// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_INCLUDE_CEP_INTERFACE_GATEWAY_H_
#define FT_INCLUDE_CEP_INTERFACE_GATEWAY_H_

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "cep/data/account.h"
#include "cep/data/config.h"
#include "cep/data/contract.h"
#include "cep/data/order.h"
#include "cep/data/position.h"
#include "cep/data/protocol.h"
#include "cep/interface/oms_interface.h"

namespace ft {

/*
 * Gateway的开发需要遵循以下规则：
 *
 * 构造函数接受TradingEngineInterface的指针，当特定行为触发时，回调engine中的函数
 * 具体规则参考TradingEngineInterface.h中的说明
 *
 * 实现Gateway基类中的虚函数，可全部实现可部分实现，根据需求而定
 *
 * 在Gateway.cpp中注册你的Gateway
 */
class Gateway {
 public:
  virtual ~Gateway() {}

  virtual bool login(OMSInterface* oms, const Config& config) { return false; }

  virtual void logout() {}

  /*
   * 发单成功返回大于0的订单号，这个订单号可传回给gateway用于撤单
   * 发单失败则返回0
   */
  virtual bool send_order(const OrderRequest& order) { return false; }

  virtual bool cancel_order(uint64_t order_id) { return false; }

  virtual bool query_contract(const std::string& ticker,
                              const std::string& exchange, Contract* result) {
    return true;
  }

  virtual bool query_contracts(std::vector<Contract>* result) { return true; }

  virtual bool query_position(const std::string& ticker, Position* result) {
    return true;
  }

  virtual bool query_positions(std::vector<Position>* result) { return true; }

  virtual bool query_account(Account* result) { return true; }

  virtual bool query_trades(std::vector<Trade>* result) { return true; }

  virtual bool query_margin_rate(const std::string& ticker) { return true; }

  virtual bool query_commision_rate(const std::string& ticker) { return true; }
};

using __GATEWAY_CREATE_FUNC = std::function<Gateway*()>;
std::map<std::string, __GATEWAY_CREATE_FUNC>& __get_api_map();
Gateway* create_gateway(const std::string& name);
void destroy_gateway(Gateway* api);

#define REGISTER_GATEWAY(name, type)                                    \
  static inline ::ft::Gateway* __create_##type() { return new type(); } \
  static inline bool __is_##type##_registered = [] {                    \
    auto& type_map = ::ft::__get_api_map();                             \
    auto res = type_map.emplace(name, __create_##type);                 \
    return res.second;                                                  \
  }()

}  // namespace ft

#endif  // FT_INCLUDE_CEP_INTERFACE_GATEWAY_H_
