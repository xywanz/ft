// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "broker/broker.h"

#include <cstdio>
#include <thread>

#include "broker/cmd_processor.h"
#include "broker/connection_manager.h"
#include "broker/session.h"
#include "core/contract_table.h"

namespace ft {

// YYYYMMDD-HH:MM:SS.sss
static time_t convert_str_to_tm(const char *str_time) {
  char year_str[5] = {'\0'};
  char mon_str[4] = {'\0'};
  char day_str[4] = {'\0'};
  struct tm tt;
  memset(&tt, 0, sizeof(tt));

  strncpy(year_str, str_time, 4);
  tt.tm_year = atoi(year_str) - 1900;

  strncpy(mon_str, str_time + 4, 2);
  tt.tm_mon = atoi(mon_str) - 1;

  strncpy(day_str, str_time + 6, 2);
  tt.tm_mday = atoi(day_str);

  tt.tm_hour = atoi(str_time + 9);
  tt.tm_min = atoi(str_time + 12);
  tt.tm_sec = atoi(str_time + 15);
  return mktime(&tt) * 1000 + atoi(str_time + 18);
}

static bool contain_between(const std::string &str, char begin, char end) {
  for (auto ch : str) {
    if (begin <= ch && ch <= end) return true;
  }

  return false;
}

/*
 * Password Policy
 * * Length is 8 characters.
 * * Must comprise of a mix of alphabets (A-Z and a-z) and digits (0-9)
 * * Must be changed on first-time logon or first logon after reset from HKEX
 *   market operations.
 * * New password can’t be one of the previous 5 passwords.
 * * Can’t be changed more than once per day.
 * * Session will be locked after 3 consecutive invalid passwords
 * * Expires every 90 days.
 */
static bool verify_passwd(const std::string &passwd) {
  if (passwd.length() != 8) return false;
  if (!contain_between(passwd, '0', '9')) return false;
  if (!contain_between(passwd, 'a', 'z')) return false;
  if (!contain_between(passwd, 'A', 'Z')) return false;
  return true;
}

Broker::Broker() {
  //   time_t t = wqutil::getWallTime();
  time_t t = time(nullptr);
  struct tm *_tm = localtime(&t);
  snprintf(date_, sizeof(date_), "%04d%02d%02d", _tm->tm_mday + 1900,
           _tm->tm_mon + 1, _tm->tm_mday);
}

uint32_t Broker::next_order_id(uint32_t ticker_index) {
  next_order_id_ += 10000;
  next_order_id_ -= next_order_id_ % 10000;
  next_order_id_ += ticker_index;
  return next_order_id_;
}

/* TODO: read config from a config file */
bool Broker::login(TradingEngineInterface *engine, const Config &config) {
  /* for test */
  {
    sess_conf_.comp_id = "CO99999902";
    sess_conf_.password = "123aA678";
    sess_conf_.new_password = "2bB56789";
    sess_conf_.rsa_pubkey_file = "./5-RSA_public_key.pem";
    sess_conf_.msg_limit_per_sec = 8;
  }

  if (!check_config()) return false;

  session_ = std::make_unique<bss::Session>(this);
  session_->init(sess_conf_);
  session_->enable();

  std::thread([this] {
    bss::ConnectionManager conn_mgr(session_.get());
    conn_mgr.run();
  }).detach();

  std::thread([this] {
    bss::CmdProcessor cmd_processor(this);
    cmd_processor.start(18889);
  }).detach();

  return true;
}

bool Broker::check_config() const {
  if (!verify_passwd(sess_conf_.password)) {
    printf("error: invalid password\n");
    return false;
  }
  if (!sess_conf_.new_password.empty() &&
      (!verify_passwd(sess_conf_.new_password) ||
       sess_conf_.password == sess_conf_.new_password)) {
    printf("error: invalid new password\n");
    return false;
  }

  return true;
}

void Broker::logon(const std::string &passwd, const std::string &new_passwd) {
  if (!verify_passwd(passwd)) {
    printf("failed to logon. invalid password\n");
    return;
  }
  if (!new_passwd.empty() &&
      (!verify_passwd(new_passwd) || passwd == new_passwd)) {
    printf("failed to logon. invalid new password\n");
    return;
  }

  session_->set_password(passwd, new_passwd);
  session_->enable();
}

void Broker::logout() { session_->disable(); }

bool Broker::send_order(const OrderReq &order) {
  if (!is_logon_) {
    printf("Broker::send_order: not logon\n");
    return false;
  }

  auto contract = order.contract;
  return true;
}

bool Broker::cancel_order(uint64_t order_id) { return true; }

bool Broker::amend_order(uint64_t order_id, const OrderReq &order) {
  return true;
}

bool mass_cancel() { return true; }

// TODO: 如果在登录成功后网络断线，密码更改成功通知没有收到，会导致本地登录密码
//       没有被更新，使得下次登录因密码错误而失败
void Broker::on_msg(const bss::LogonMessage &msg) {
  next_order_id_ = msg.next_expected_message_sequence;
  is_logon_ = true;
  if (msg.session_status == bss::BssSessionStatus::SESSION_PASSWORD_CHANGE) {
    sess_conf_.password = sess_conf_.new_password;
    sess_conf_.new_password = "";
    session_->set_password(sess_conf_.password, sess_conf_.new_password);
  }
}

void Broker::on_msg(const bss::LogoutMessage &msg) { is_logon_ = false; }

void Broker::on_msg(const bss::RejectMessage &msg) {
  printf("Broker::on_session_rejected: RejectReason:%d Reason:%s\n",
         msg.message_reject_code, msg.reason.data);
  uint32_t client_order_id = atoi(msg.client_order_id);
}

void Broker::on_msg(const bss::BusinessRejectMessage &msg) {
  printf("Broker::on_business_rejected: RejectReason:%d Reason:%s\n",
         msg.business_reject_code, msg.reason.data);
  uint32_t client_order_id = atoi(msg.business_reject_reference_id);
}

void Broker::on_msg(const bss::ExecutionReport &report) {
  switch (report.exec_type) {
    case bss::OcgExecType::EXEC_TYPE_NEW: {
      on_order_accepted(report);
      break;
    }
    case bss::OcgExecType::EXEC_TYPE_CANCEL: {
      on_order_cancelled(report);
      break;
    }
    case bss::OcgExecType::EXEC_TYPE_CANCEL_REJECT: {
      on_order_cancel_rejected(report);
      break;
    }
    case bss::OcgExecType::EXEC_TYPE_REJECT: {
      on_order_rejected(report);
      break;
    }
    case bss::OcgExecType::EXEC_TYPE_AMEND: {
      on_order_amended(report);
      break;
    }
    case bss::OcgExecType::EXEC_TYPE_AMEND_REJECT: {
      on_order_amend_rejected(report);
      break;
    }
    case bss::OcgExecType::EXEC_TYPE_EXPIRE: {
      on_order_expired(report);
      break;
    }
    case bss::OcgExecType::EXEC_TYPE_TRADE: {
      on_order_executed(report);
      break;
    }
    case bss::OcgExecType::EXEC_TYPE_TRADE_CANCEL: {
    }
    default: {
      printf("Broker::on_msg: not support this ExecType[%d]\n",
             report.exec_type);
      break;
    }
  }
}

void Broker::on_msg(const bss::OrderMassCancelReport &report) {}

void Broker::on_msg(const bss::QuoteStatusReport &report) {}

void Broker::on_msg(const bss::TradeCaptureReport &report) {}

void Broker::on_msg(const bss::TradeCaptureReportAck &ack) {}

void Broker::on_order_accepted(const bss::ExecutionReport &msg) {
  printf(
      "Broker::on_order_accepted: ClientOrderID[%s] security[%s] OrderQty[%lu] "
      "Price[%lf]\n",
      msg.client_order_id, msg.security_id, msg.order_quantity / 100000000,
      static_cast<double>(msg.price) / 1e8);

  uint32_t client_order_id = atoi(msg.client_order_id);
}

void Broker::on_order_rejected(const bss::ExecutionReport &msg) {
  printf("Broker::on_order_rejected: RejectReason:%d Reason:%s\n",
         msg.order_reject_code, msg.reason.data);
  uint32_t client_order_id = atoi(msg.client_order_id);
}

void Broker::on_order_executed(const bss::ExecutionReport &msg) {
  int qty = msg.execution_quantity / 100000000;
  double price = static_cast<double>(msg.execution_price) / 1e8;
  printf(
      "Broker::on_order_executed: OrderID[%s] ClOrdToken[%s] LastQty[%d] "
      "LastPrice[%lf]\n",
      msg.order_id, msg.client_order_id, qty, price);
  uint32_t client_order_id = atoi(msg.client_order_id);
}

void Broker::on_order_cancelled(const bss::ExecutionReport &msg) {
  int qty = msg.execution_quantity / 100000000;

  printf(
      "Broker::on_order_cancelled: ClientOrderID[%s] OriginalOrderID[%s] "
      "FilledQty[%d] LeavesQty[%lu]\n",
      msg.client_order_id, msg.original_client_order_id, qty,
      msg.leaves_quantity / 100000000);
  uint32_t client_order_id = atoi(msg.client_order_id);
}

void Broker::on_order_cancel_rejected(const bss::ExecutionReport &msg) {
  printf("Broker::on_order_cancel_rejected: RejectReason:%d Reason:%s\n",
         msg.cancel_reject_code, msg.reason.data);
  uint32_t client_order_id = atoi(msg.original_client_order_id);
}

void Broker::on_order_amended(const bss::ExecutionReport &msg) {
  int qty = msg.execution_quantity / 100000000;
  double price = static_cast<double>(msg.execution_price) / 1e8;
  printf(
      "Broker::on_order_amend: ClientOrderID[%s] OriginalOrderID[%s] Qty[%d] "
      "Price[%lf]\n",
      msg.client_order_id, msg.original_client_order_id, qty, price);
  uint32_t client_order_id = atoi(msg.original_client_order_id);
}

void Broker::on_order_amend_rejected(const bss::ExecutionReport &msg) {
  printf("Broker::on_order_amend_rejected: RejectReason:%d Reason:%s\n",
         msg.amend_reject_code, msg.reason.data);
  uint32_t client_order_id = atoi(msg.original_client_order_id);
}

void Broker::on_order_expired(const bss::ExecutionReport &msg) {
  printf("Broker::on_order_expired: RejectReason:%d Reason:%s\n",
         msg.order_reject_code, msg.reason.data);
  uint32_t client_order_id = atoi(msg.client_order_id);
}

}  // namespace ft
