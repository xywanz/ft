// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_SRC_TRADER_GATEWAY_GATEWAY_H_
#define FT_SRC_TRADER_GATEWAY_GATEWAY_H_

#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "ft/base/config.h"
#include "ft/base/market_data.h"
#include "ft/base/trade_msg.h"
#include "ft/utils/ring_buffer.h"
#include "trader/msg.h"

namespace ft {

// Gateway是所有交易网关的基类，目前行情、查询、交易类的接口都
// 集成在了Gateway中，后续会把行情及交易无关的查询剥离到其他接
// 口中。Gateway会被OMS创建并使用，OMS会在登录、登出、查询时保
// 证Gateway线程安全，对于这几个函数，Gateway无需另外实现一套线
// 程安全的机制，而对于发单及撤单函数，OMS并没有保证线程安全，同
// 一时刻可能存在着多个调用，所以Gateway实现时要注意发单及撤单
// 函数的线程安全性
//
// Gateway应该实现成一个简单的订单路由类，任务是把订单转发到相应
// 的broker或交易所，并把订单回报以相应的规则传回给OMS，内部应该
// 尽可能地做到无锁实现。
//
// 接口说明
// 1. 查询接口都应该实现成同步的接口
// 2. 交易及行情都应该实现成异步的接口
// 3. 收到回报时应该回调相应的OMS函数，以告知OMS
//
// 在gateway.cpp中注册你的Gateway
class Gateway {
 public:
  using OrderRspRB = ft::RingBuffer<GatewayOrderResponse, 1024>;
  using QryReultRB = ft::RingBuffer<GatewayQueryResult, 1024>;
  using TickRB = ft::RingBuffer<TickData, 4096>;

 public:
  virtual ~Gateway() {}

  // 登录函数。在登录函数中，gateway应当保存oms指针，以供后续回调
  // 使用。同时，login函数成功执行后，gateway即进入到了可交易的状
  // 态（如果配置了交易服务器的话），或是进入到了可订阅行情数据的
  // 状态（如果配置了行情服务器的话）
  //
  // login最好实现成可被多次调用，即和logout配合使用，可反复地主动
  // 登录登出
  virtual bool Init(const GatewayConfig& config) { return false; }

  // 登出函数。从服务器登出，以达到禁止交易或中断行情的目的。外部
  // 可通过logout来暂停交易。
  virtual void Logout() {}

  // 发送订单，privdata_ptr是外部传入的指针，gateway可以把该订单
  // 相关的私有数据，交由外部保存，撤单时外部会将privdata传回给
  // gateway
  virtual bool SendOrder(const OrderRequest& order, uint64_t* privdata_ptr) { return false; }

  virtual bool CancelOrder(uint64_t order_id, uint64_t privdata) { return false; }

  virtual bool Subscribe(const std::vector<std::string>& sub_list) { return false; }

  virtual bool QueryContracts() { return false; }

  virtual bool QueryPositions() { return false; }

  virtual bool QueryAccount() { return false; }

  virtual bool QueryOrders() { return false; }

  virtual bool QueryTrades() { return false; }

  // 扩展接口，用于向Gateway发送自定义消息
  virtual void OnNotify(uint64_t signal) {}

  OrderRspRB* GetOrderRspRB() { return &rsp_rb_; }

  QryReultRB* GetQryResultRB() { return &qry_result_rb_; }

  TickRB* GetTickRB() { return &tick_rb_; }

 protected:
  void OnOrderAccepted(const OrderAcceptedRsp& rsp);

  void OnOrderTraded(const OrderTradedRsp& rsp);

  void OnOrderRejected(const OrderRejectedRsp& rsp);

  void OnOrderCanceled(const OrderCanceledRsp& rsp);

  void OnOrderCancelRejected(const OrderCancelRejectedRsp& rsp);

  void OnQueryAccount(const Account& rsp);

  void OnQueryAccountEnd();

  void OnQueryPosition(const Position& rsp);

  void OnQueryPositionEnd();

  void OnQueryOrder(const HistoricalOrder& rsp);

  void OnQueryOrderEnd();

  void OnQueryTrade(const HistoricalTrade& rsp);

  void OnQueryTradeEnd();

  void OnQueryContract(const Contract& rsp);

  void OnQueryContractEnd();

  void OnTick(const TickData& tick_data);

 private:
  OrderRspRB rsp_rb_;
  QryReultRB qry_result_rb_;
  TickRB tick_rb_;
};

inline void Gateway::OnOrderAccepted(const OrderAcceptedRsp& rsp) {
  GatewayOrderResponse gtw_rsp;
  gtw_rsp.msg_type = GatewayMsgType::kOrderAcceptedRsp;
  gtw_rsp.data = rsp;
  rsp_rb_.PutWithBlocking(std::move(gtw_rsp));
}

inline void Gateway::OnOrderTraded(const OrderTradedRsp& rsp) {
  GatewayOrderResponse gtw_rsp;
  gtw_rsp.msg_type = GatewayMsgType::kOrderOrderTradedRsp;
  gtw_rsp.data = rsp;
  rsp_rb_.PutWithBlocking(std::move(gtw_rsp));
}

inline void Gateway::OnOrderRejected(const OrderRejectedRsp& rsp) {
  GatewayOrderResponse gtw_rsp;
  gtw_rsp.msg_type = GatewayMsgType::kOrderRejectedRsp;
  gtw_rsp.data = rsp;
  rsp_rb_.PutWithBlocking(std::move(gtw_rsp));
}

inline void Gateway::OnOrderCanceled(const OrderCanceledRsp& rsp) {
  GatewayOrderResponse gtw_rsp;
  gtw_rsp.msg_type = GatewayMsgType::kOrderCanceledRsp;
  gtw_rsp.data = rsp;
  rsp_rb_.PutWithBlocking(std::move(gtw_rsp));
}

inline void Gateway::OnOrderCancelRejected(const OrderCancelRejectedRsp& rsp) {
  GatewayOrderResponse gtw_rsp;
  gtw_rsp.msg_type = GatewayMsgType::kOrderCancelRejectedRsp;
  gtw_rsp.data = rsp;
  rsp_rb_.PutWithBlocking(std::move(gtw_rsp));
}

inline void Gateway::OnQueryAccount(const Account& rsp) {
  GatewayQueryResult gtw_rsp;
  gtw_rsp.msg_type = GatewayMsgType::kAccount;
  gtw_rsp.data = rsp;
  qry_result_rb_.PutWithBlocking(gtw_rsp);
}

inline void Gateway::OnQueryAccountEnd() {
  GatewayQueryResult gtw_rsp;
  gtw_rsp.msg_type = GatewayMsgType::kAccountEnd;
  qry_result_rb_.PutWithBlocking(gtw_rsp);
}

inline void Gateway::OnQueryPosition(const Position& rsp) {
  GatewayQueryResult gtw_rsp;
  gtw_rsp.msg_type = GatewayMsgType::kPosition;
  gtw_rsp.data = rsp;
  qry_result_rb_.PutWithBlocking(gtw_rsp);
}

inline void Gateway::OnQueryPositionEnd() {
  GatewayQueryResult gtw_rsp;
  gtw_rsp.msg_type = GatewayMsgType::kPositionEnd;
  qry_result_rb_.PutWithBlocking(gtw_rsp);
}

inline void Gateway::OnQueryOrder(const HistoricalOrder& rsp) {
  GatewayQueryResult gtw_rsp;
  gtw_rsp.msg_type = GatewayMsgType::kOrder;
  gtw_rsp.data = rsp;
  qry_result_rb_.PutWithBlocking(gtw_rsp);
}

inline void Gateway::OnQueryOrderEnd() {
  GatewayQueryResult gtw_rsp;
  gtw_rsp.msg_type = GatewayMsgType::kOrderEnd;
  qry_result_rb_.PutWithBlocking(gtw_rsp);
}

inline void Gateway::OnQueryTrade(const HistoricalTrade& rsp) {
  GatewayQueryResult gtw_rsp;
  gtw_rsp.msg_type = GatewayMsgType::kTrade;
  gtw_rsp.data = rsp;
  qry_result_rb_.PutWithBlocking(gtw_rsp);
}

inline void Gateway::OnQueryTradeEnd() {
  GatewayQueryResult gtw_rsp;
  gtw_rsp.msg_type = GatewayMsgType::kTradeEnd;
  qry_result_rb_.PutWithBlocking(gtw_rsp);
}

inline void Gateway::OnQueryContract(const Contract& rsp) {
  GatewayQueryResult gtw_rsp;
  gtw_rsp.msg_type = GatewayMsgType::kContract;
  gtw_rsp.data = rsp;
  qry_result_rb_.PutWithBlocking(gtw_rsp);
}

inline void Gateway::OnQueryContractEnd() {
  GatewayQueryResult gtw_rsp;
  gtw_rsp.msg_type = GatewayMsgType::kContractEnd;
  qry_result_rb_.PutWithBlocking(gtw_rsp);
}

inline void Gateway::OnTick(const TickData& tick_data) { tick_rb_.PutWithBlocking(tick_data); }

std::shared_ptr<Gateway> CreateGateway(const std::string& name);

}  // namespace ft

#endif  // FT_SRC_TRADER_GATEWAY_GATEWAY_H_
