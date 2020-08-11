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
 * Gateway是所有交易网关的基类，目前行情、查询、交易类的接口都
 * 集成在了Gateway中，后续会把行情及交易无关的查询剥离到其他接
 * 口中。Gateway会被OMS创建并使用，OMS会在登录、登出、查询时保
 * 证Gateway线程安全，对于这几个函数，Gateway无需另外实现一套线
 * 程安全的机制，而对于发单及撤单函数，OMS并没有保证线程安全，同
 * 一时刻可能存在着多个调用，所以Gateway实现时要注意发单及撤单
 * 函数的线程安全性
 *
 * Gateway应该实现成一个简单的订单路由类，任务是把订单转发到相应
 * 的broker或交易所，并把订单回报以相应的规则传回给OMS，内部应该
 * 尽可能地做到无锁实现。
 *
 * 接口说明
 * 1. 查询接口都应该实现成同步的接口
 * 2. 交易及行情都应该实现成异步的接口
 * 3. 收到回报时应该回调相应的OMS函数，以告知OMS
 *
 * 在gateway.cpp中注册你的Gateway
 */
class Gateway {
 public:
  virtual ~Gateway() {}

  /*
   * 登录函数。在登录函数中，gateway应当保存oms指针，以供后续回调
   * 使用。同时，login函数成功执行后，gateway即进入到了可交易的状
   * 态（如果配置了交易服务器的话），或是进入到了可订阅行情数据的
   * 状态（如果配置了行情服务器的话）
   *
   * login最好实现成可被多次调用，即和logout配合使用，可反复地主动
   * 登录登出
   */
  virtual bool login(OMSInterface* oms, const Config& config) { return false; }

  /*
   * 登出函数。从服务器登出，以达到禁止交易或中断行情的目的。外部
   * 可通过logout来暂停交易。
   */
  virtual void logout() {}

  /*
   * 发送订单，privdata_ptr是外部传入的指针，gateway可以把该订单
   * 相关的私有数据，交由外部保存，撤单时外部会将privdata传回给
   * gateway
   */
  virtual bool send_order(const OrderRequest& order, uint64_t* privdata_ptr) {
    return false;
  }

  virtual bool cancel_order(uint64_t order_id, uint64_t privdata) {
    return false;
  }

  virtual bool subscribe(const std::vector<std::string>& sub_list) {
    return true;
  }

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
