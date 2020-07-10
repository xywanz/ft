// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include <memory>
#include <string>

#include "broker/session_config.h"
#include "interface/gateway.h"
#include "protocol/protocol.h"

#ifndef BSS_BROKER_BROKER_H_
#define BSS_BROKER_BROKER_H_

namespace ft {

namespace bss {
class Session;
}

/*
 * Broker类用于处理业务逻辑
 *
 * Broker所收到的消息都是连续且正确的，Broker内部只需要专注于处理业务逻辑
 */
class Broker : public Gateway {
 public:
  Broker();

  bool login(TradingEngineInterface* engine, const Config& config);

  void logon(const std::string& passwd, const std::string& new_passwd = "");

  void logout();

  uint32_t next_order_id(uint32_t ticker_index);

  bool send_order(const OrderReq& order) override;

  bool cancel_order(uint64_t order_id) override;

  bool amend_order(uint64_t order_id, const OrderReq& order);

  bool mass_cancel();

 public:
  void on_order_accepted(const bss::ExecutionReport& msg);

  void on_order_rejected(const bss::ExecutionReport& msg);

  void on_order_executed(const bss::ExecutionReport& msg);

  void on_order_cancelled(const bss::ExecutionReport& msg);

  void on_order_cancel_rejected(const bss::ExecutionReport& msg);

  void on_order_amended(const bss::ExecutionReport& msg);

  void on_order_amend_rejected(const bss::ExecutionReport& msg);

  void on_order_expired(const bss::ExecutionReport& msg);

 public:
  void on_msg(const bss::LogonMessage& msg);

  void on_msg(const bss::LogoutMessage& msg);

  void on_msg(const bss::RejectMessage& msg);

  void on_msg(const bss::BusinessRejectMessage& msg);

  void on_msg(const bss::ExecutionReport& report);

  void on_msg(const bss::OrderMassCancelReport& report);

  void on_msg(const bss::QuoteStatusReport& report);

  void on_msg(const bss::TradeCaptureReport& report);

  void on_msg(const bss::TradeCaptureReportAck& ack);

 private:
  void get_transaction_time(bss::TransactionTime trans_time) {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    int hour = static_cast<int>((ts.tv_sec % 86400 / 3600));
    int min = static_cast<int>((ts.tv_sec / 60) % 60);
    int sec = static_cast<int>(ts.tv_sec % 60);
    snprintf(trans_time, sizeof(bss::TransactionTime), "%s-%02d:%02d:%02d.%03d",
             date_, hour, min, sec, static_cast<int>(ts.tv_nsec / 1000000));
  }

  bool check_config() const;

 private:
  bss::SessionConfig sess_conf_;
  bool is_logon_{false};
  std::unique_ptr<bss::Session> session_;
  bss::BrokerId m_oBrokerId;
  uint32_t next_order_id_{1};
  char date_[32]{};
};

}  // namespace ft

#endif  // BSS_BROKER_BROKER_H_
