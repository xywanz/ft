// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_INCLUDE_CORE_GATEWAY_H_
#define FT_INCLUDE_CORE_GATEWAY_H_

#include <functional>
#include <map>
#include <memory>
#include <string>

#include "core/config.h"
#include "core/protocol.h"
#include "core/trading_engine_interface.h"

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

  /*
   * 一个Gateway只能登录一次，并不保证重复调用或同时调用能产生正确的行为
   */
  virtual bool login(const Config& config) { return false; }

  virtual void logout() {}

  /*
   * 发单成功返回大于0的订单号，这个订单号可传回给gateway用于撤单
   * 发单失败则返回0
   */
  virtual bool send_order(const OrderReq* order) { return false; }

  virtual bool cancel_order(uint64_t order_id) { return false; }

  virtual bool query_contract(const std::string& ticker,
                              const std::string& exchange) {
    return true;
  }

  virtual bool query_contracts() { return true; }

  virtual bool query_position(const std::string& ticker) { return true; }

  virtual bool query_positions() { return true; }

  virtual bool query_account() { return true; }

  virtual bool query_trades() { return true; }

  virtual bool query_margin_rate(const std::string& ticker) { return true; }

  virtual bool query_commision_rate(const std::string& ticker) { return true; }
};

using __GATEWAY_CREATE_FUNC = std::function<Gateway*(TradingEngineInterface*)>;
std::map<std::string, __GATEWAY_CREATE_FUNC>& __get_api_map();
Gateway* create_gateway(const std::string& name,
                        TradingEngineInterface* engine);
void destroy_gateway(Gateway* api);

#define REGISTER_GATEWAY(name, type)                    \
  static inline ::ft::Gateway* __create_##type(         \
      ::ft::TradingEngineInterface* engine) {           \
    return new type(engine);                            \
  }                                                     \
  static inline bool __is_##type##_registered = [] {    \
    auto& type_map = ::ft::__get_api_map();             \
    auto res = type_map.emplace(name, __create_##type); \
    return res.second;                                  \
  }()

}  // namespace ft

#endif  // FT_INCLUDE_CORE_GATEWAY_H_
