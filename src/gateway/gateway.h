// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_SRC_GATEWAY_GATEWAY_H_
#define FT_SRC_GATEWAY_GATEWAY_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "protocol/data_types.h"
#include "trading_server/common/config.h"
#include "trading_server/order_management/base_oms.h"

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
  virtual bool Login(BaseOrderManagementSystem* oms, const Config& config) { return false; }

  /*
   * 登出函数。从服务器登出，以达到禁止交易或中断行情的目的。外部
   * 可通过logout来暂停交易。
   */
  virtual void Logout() {}

  /*
   * 发送订单，privdata_ptr是外部传入的指针，gateway可以把该订单
   * 相关的私有数据，交由外部保存，撤单时外部会将privdata传回给
   * gateway
   */
  virtual bool SendOrder(const OrderRequest& order, uint64_t* privdata_ptr) { return false; }

  virtual bool CancelOrder(uint64_t order_id, uint64_t privdata) { return false; }

  virtual bool Subscribe(const std::vector<std::string>& sub_list) { return true; }

  virtual bool QueryContract(const std::string& ticker, const std::string& exchange,
                             Contract* result) {
    return true;
  }

  virtual bool QueryContractList(std::vector<Contract>* result) { return true; }

  virtual bool QueryPosition(const std::string& ticker, Position* result) { return true; }

  virtual bool QueryPositionList(std::vector<Position>* result) { return true; }

  virtual bool QueryAccount(Account* result) { return true; }

  virtual bool QueryTradeList(std::vector<Trade>* result) { return true; }

  virtual bool QueryMarginRate(const std::string& ticker) { return true; }

  virtual bool QueryCommisionRate(const std::string& ticker) { return true; }

  // 扩展接口，用于向Gateway发送自定义消息
  virtual void OnNotify(uint64_t signal) {}
};

using GatewayCreateFunc = Gateway* (*)();
using GatewayDestroyFunc = void (*)(Gateway*);

#define REGISTER_GATEWAY(type)                                     \
  extern "C" ::ft::Gateway* CreateGateway() { return new type(); } \
  extern "C" void DestroyGateway(::ft::Gateway* p) { delete p; }

}  // namespace ft

#endif  // FT_SRC_GATEWAY_GATEWAY_H_
