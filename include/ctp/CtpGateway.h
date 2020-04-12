// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_INCLUDE_CTP_CTPGATEWAY_H_
#define FT_INCLUDE_CTP_CTPGATEWAY_H_

#include <atomic>
#include <cstring>
#include <map>
#include <mutex>
#include <string>
#include <vector>

#include <fmt/format.h>
#include <ThostFtdcTraderApi.h>

#include "LoginParams.h"
#include "GatewayInterface.h"
#include "Order.h"
#include "TraderInterface.h"

namespace ft {

class CtpGateway : public GatewayInterface {
 public:
  CtpGateway();

  ~CtpGateway();

  void register_cb(TraderInterface* trader) override {
    trader_ = trader;
  }

  bool login(const LoginParams& params) override;

  bool logout() {
  }

  std::string send_order(const Order* order) override;

  bool cancel_order(const std::string& order_id) override;

  AsyncStatus query_contract(const std::string& symbol,
                             const std::string& exchange) override;

  AsyncStatus query_position(const std::string& symbol,
                             const std::string& exchange) override;

  AsyncStatus query_account() override;

  void join() override {
    if (is_login_)
      api_->Join();
  }


  // 当客户端与交易后台建立起通信连接时（还未登录前），该方法被调用。
  void on_connected();

  // 当客户端与交易后台通信连接断开时，该方法被调用。
  // 当发生这个情况后，API会自动重新连接，客户端可不做处理。
  // @param reason 错误原因
  //         0x1001 网络读失败
  //         0x1002 网络写失败
  //         0x2001 接收心跳超时
  //         0x2002 发送心跳失败
  //         0x2003 收到错误报文
  void on_disconnected(int reason);

  // 心跳超时警告。当长时间未收到报文时，该方法被调用。
  // @param time_lapse 距离上次接收报文的时间
  void on_heart_beat_warning(int time_lapse);

  // 客户端认证响应
  void on_authenticate(CThostFtdcRspAuthenticateField *rsp_authenticate_field,
                       CThostFtdcRspInfoField *rsp_info,
                       int req_id,
                       bool is_last);

  // 登录请求响应
  void on_login(CThostFtdcRspUserLoginField *rsp_user_login,
                CThostFtdcRspInfoField *rsp_info,
                int req_id,
                bool is_last);

  void on_settlement(CThostFtdcSettlementInfoField *settlement_info,
                     CThostFtdcRspInfoField *rsp_info,
                     int req_id,
                     bool is_last);

  void on_settlement_confirm(
          CThostFtdcSettlementInfoConfirmField *settlement_info_confirm,
          CThostFtdcRspInfoField *rsp_info,
          int req_id,
          bool is_last);

  // 拒绝报单
  void on_order_rejected(CThostFtdcInputOrderField *ctp_order,
                         CThostFtdcRspInfoField *rsp_info,
                         int req_id,
                         bool is_last);

  void on_order_action(CThostFtdcInputOrderActionField *action,
                       CThostFtdcRspInfoField *rsp_info,
                       int req_id,
                       bool is_last);

  void on_order(CThostFtdcOrderField *ctp_order);

  // 成交通知
  void on_trade(CThostFtdcTradeField *trade);

  void on_contract(CThostFtdcInstrumentField *instrument,
                   CThostFtdcRspInfoField *rsp_info,
                   int req_id,
                   bool is_last);

  void on_position(
          CThostFtdcInvestorPositionField *position,
          CThostFtdcRspInfoField *rsp_info,
          int req_id,
          bool is_last);

  void on_account(CThostFtdcTradingAccountField *trading_account,
                  CThostFtdcRspInfoField *rsp_info,
                  int req_id,
                  bool is_last);

 private:
  AsyncStatus req_async_status(int req_id) {
    std::unique_lock<std::mutex> lock(status_mutex_);
    auto res = req_status_.emplace(req_id, AsyncStatus{true});
    return res.first->second;
  }

  void rsp_async_status(int req_id, bool success) {
    std::unique_lock<std::mutex> lock(status_mutex_);
    auto status = req_status_[req_id];
    req_status_.erase(req_id);
    lock.unlock();
    if (success)
      status.set_success();
    else
      status.set_error();
  }

  int next_req_id() {
    return next_req_id_++;
  }

  int next_order_ref() {
    return next_order_ref_++;
  }

  std::string get_order_id(const char* order_ref) {
    return fmt::format("{}_{}_{}", front_id_, session_id_, order_ref);
  }

  AsyncStatus query_settlement();
  AsyncStatus req_settlement_confirm();

 private:
  TraderInterface* trader_;

  CThostFtdcTraderApi* api_ = nullptr;
  CThostFtdcTraderSpi* spi_ = nullptr;

  std::string front_addr_;
  std::string broker_id_;
  std::string investor_id_;
  int front_id_ = 0;
  int session_id_ = 0;

  std::atomic<int> next_req_id_ = 0;
  std::atomic<int> next_order_ref_ = 0;

  std::atomic<bool> is_connected_ = false;
  std::atomic<bool> is_login_ = false;

  std::map<int, AsyncStatus> req_status_;
  std::mutex status_mutex_;

  std::map<std::string, Order> id2order_;
  std::mutex order_mutex_;

  std::map<int, std::map<std::string, Position>> pos_caches_;
};

}  // namespace ft

#endif  // FT_INCLUDE_CTP_CTPGATEWAY_H_
