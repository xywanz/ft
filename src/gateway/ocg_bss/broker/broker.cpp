// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "broker/broker.h"

#include <spdlog/spdlog.h>

#include <cstdio>
#include <thread>

#include "broker/cmd_processor.h"
#include "broker/connection_manager.h"
#include "broker/session.h"
#include "trading_server/datastruct/contract.h"
#include "trading_server/datastruct/contract_table.h"

namespace ft {

// YYYYMMDD-HH:MM:SS.sss
static time_t convert_str_to_tm(const char *str_time) {
  char year_str[5];
  char mon_str[4];
  char day_str[4];
  struct tm tt {};

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

BssBroker::BssBroker() {
  //   time_t t = wqutil::getWallTime();
  time_t t = time(nullptr);
  struct tm _tm;
  localtime_r(&t, &_tm);
  snprintf(date_, sizeof(date_), "%04d%02d%02d", _tm.tm_mday + 1900, _tm.tm_mon + 1, _tm.tm_mday);
}

/* TODO: read config from a config file */
bool BssBroker::Login(BaseOrderManagementSystem *oms, const Config &config) {
  oms_ = oms;
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
  session_->Init(sess_conf_);
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

bool BssBroker::check_config() const {
  if (!verify_passwd(sess_conf_.password)) {
    spdlog::error("[BssBroker::check_config] invalid password");
    return false;
  }
  if (!sess_conf_.new_password.empty() &&
      (!verify_passwd(sess_conf_.new_password) || sess_conf_.password == sess_conf_.new_password)) {
    spdlog::error("[BssBroker::check_config] invalid new password");
    return false;
  }

  return true;
}

void BssBroker::logon(const std::string &passwd, const std::string &new_passwd) {
  if (!verify_passwd(passwd)) {
    spdlog::error("[BssBroker::logon] invalid password");
    return;
  }
  if (!new_passwd.empty() && (!verify_passwd(new_passwd) || passwd == new_passwd)) {
    spdlog::error("[BssBroker::logon] invalid new password");
    return;
  }

  session_->set_password(passwd, new_passwd);
  session_->enable();
}

void BssBroker::Logout() { session_->disable(); }

bool BssBroker::QueryAccount(Account *result) {
  // test
  {
    result->account_id = 1234;
    result->total_asset = result->cash = 10000000;
  }

  return true;
}

bool BssBroker::SendOrder(const OrderRequest &order, uint64_t *privdata_ptr) {
  (void)privdata_ptr;

  if (!is_logon_) {
    spdlog::error("[BssBroker::SendOrder]: not logon");
    return false;
  }

  auto contract = order.contract;

  bss::NewOrderRequest req{};
  snprintf(req.client_order_id, sizeof(req.client_order_id), "%u",
           static_cast<uint32_t>(order.order_id));
  strncpy(req.submitting_broker_id, broker_id_, sizeof(req.submitting_broker_id));
  strncpy(req.security_id, contract->ticker.c_str(), sizeof(req.security_id));
  req.side = bss_detail::diroff2side(order.direction, order.offset);
  switch (order.type) {
    case ::ft::OrderType::hkex::MO_AT_CROSSING: {
      req.tif = bss::TIF_AT_CROSSING;
      req.order_type = bss::ORDER_TYPE_MARKET;
      break;
    }
    case ::ft::OrderType::hkex::LO_AT_CROSSING: {
      req.tif = bss::TIF_AT_CROSSING;
      req.order_type = bss::ORDER_TYPE_LIMIT;
      break;
    }
    case ::ft::OrderType::hkex::LO: {
      req.tif = bss::TIF_DAY;
      req.order_type = bss::ORDER_TYPE_LIMIT;
      req.max_price_levels = 1;
      break;
    }
    case ::ft::OrderType::hkex::ELO: {
      req.tif = bss::TIF_DAY;
      req.order_type = bss::ORDER_TYPE_LIMIT;
      break;
    }
    case ::ft::OrderType::hkex::SLO: {
      req.tif = bss::TIF_IOC;
      req.order_type = bss::ORDER_TYPE_LIMIT;
      break;
    }
    default: {
      return false;
    }
  }
  if (req.order_type == bss::ORDER_TYPE_LIMIT) req.price = (order.price + 0.0005) * 1e3 * 1e5;

  req.order_quantity = order.volume * 1e8;
  req.security_id_source = 8;
  snprintf(req.security_exchange, sizeof(req.security_exchange), "%s", "XHKG");
  get_transaction_time(req.transaction_time);
  // todo
  req.position_effect = 0;
  req.disclosure_instructions = 0;

  if (!session_->send_business_msg(req)) {
    spdlog::error("[BssBroker::SendOrder] failed to send order");
    return false;
  }
  return true;
}

bool BssBroker::CancelOrder(uint64_t order_id, uint64_t privdata) { return true; }

bool BssBroker::amend_order(uint64_t order_id, const OrderRequest &order) { return true; }

bool mass_cancel() { return true; }

// TODO(kevin):
// 如果在登录成功后网络断线，密码更改成功通知没有收到，
// 会导致本地登录密码没有被更新，使得下次登录因密码错误而失败
void BssBroker::on_msg(const bss::LogonMessage &msg) {
  is_logon_ = true;
  if (msg.session_status == bss::BssSessionStatus::SESSION_PASSWORD_CHANGE) {
    sess_conf_.password = sess_conf_.new_password;
    sess_conf_.new_password = "";
    session_->set_password(sess_conf_.password, sess_conf_.new_password);
  }
  if (msg.text.len > 0) spdlog::info("logon text: {}", msg.text.data);
}

void BssBroker::on_msg(const bss::LogoutMessage &msg) {
  is_logon_ = false;
  if (msg.session_status == bss::BssSessionStatus::INVAILD_USERNAME_OR_PASSWORD ||
      msg.session_status == bss::BssSessionStatus::ACCOUNT_LOCKED ||
      msg.session_status == bss::BssSessionStatus::LOGONS_NOT_ALLOWED_AT_THIS_TIME ||
      msg.session_status == bss::BssSessionStatus::PASSWORD_EXPIRED ||
      msg.session_status == bss::BssSessionStatus::PASSWORD_CHANGE_REQUIRED) {
    session_->disable();
  }
  if (msg.logout_text.len > 0) spdlog::info("Logout text: {}", msg.logout_text.data);
}

void BssBroker::on_msg(const bss::RejectMessage &msg) {
  spdlog::error("[BssBroker::on_session_rejected] RejectCode:{} Reason:{}", msg.message_reject_code,
                msg.reason.data);
  uint32_t client_order_id = atoi(msg.client_order_id);
}

void BssBroker::on_msg(const bss::BusinessRejectMessage &msg) {
  spdlog::error("[BssBroker::on_business_rejected] RejectCode:{} Reason:{}",
                msg.business_reject_code, msg.reason.data);
  uint32_t client_order_id = atoi(msg.business_reject_reference_id);
}

void BssBroker::on_msg(const bss::ExecutionReport &report) {
  switch (report.exec_type) {
    case bss::OcgExecType::EXEC_TYPE_NEW: {
      OnOrderAccepted(report);
      break;
    }
    case bss::OcgExecType::EXEC_TYPE_CANCEL: {
      on_order_cancelled(report);
      break;
    }
    case bss::OcgExecType::EXEC_TYPE_CANCEL_REJECT: {
      OnOrderCancelRejected(report);
      break;
    }
    case bss::OcgExecType::EXEC_TYPE_REJECT: {
      OnOrderRejected(report);
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
      spdlog::error("[BssBroker::on_ep]: exec type not supported. type:{}", report.exec_type);
      break;
    }
  }
}

void BssBroker::on_msg(const bss::OrderMassCancelReport &report) {}

void BssBroker::on_msg(const bss::QuoteStatusReport &report) {}

void BssBroker::on_msg(const bss::TradeCaptureReport &report) {}

void BssBroker::on_msg(const bss::TradeCaptureReportAck &ack) {}

void BssBroker::OnOrderAccepted(const bss::ExecutionReport &msg) {
  spdlog::debug(
      "[BssBroker::OnOrderAccepted] ClientOrderID:{} Security:{} "
      "OrderQty:{} Price:{}",
      msg.client_order_id, msg.security_id, msg.order_quantity / 100000000,
      static_cast<double>(msg.price) / 1e8);

  OrderAcceptance rsp{};
  rsp.order_id = std::stoul(msg.client_order_id);
  // rsp.order_id = std::stoul(msg.order_id);
  oms_->OnOrderAccepted(&rsp);
}

void BssBroker::OnOrderRejected(const bss::ExecutionReport &msg) {
  spdlog::error("[BssBroker::OnOrderRejected] RejectCode:{} Reason:{}", msg.order_reject_code,
                msg.reason.data);

  OrderRejection rsp{};
  rsp.order_id = std::stoul(msg.client_order_id);
  oms_->OnOrderRejected(&rsp);
}

void BssBroker::on_order_executed(const bss::ExecutionReport &msg) {
  int qty = msg.execution_quantity / 100000000;
  double price = static_cast<double>(msg.execution_price) / 1e8;
  spdlog::debug(
      "[BssBroker::on_order_executed] OrderID:{} ClOrdToken:{} LastQty:{} "
      "LastPrice:{}",
      msg.order_id, msg.client_order_id, qty, price);

  Trade rsp{};
  rsp.order_id = std::stoul(msg.client_order_id);
  // rsp.order_id = std::stoul(msg.order_id);
  rsp.volume = qty;
  rsp.price = price;
  rsp.trade_type = TradeType::SECONDARY_MARKET;
  oms_->OnOrderTraded(&rsp);
}

void BssBroker::on_order_cancelled(const bss::ExecutionReport &msg) {
  // TODO(kevin): 可能没有order_quantity这个字段
  int total = msg.order_quantity / 100000000;
  int traded = msg.cumulative_quantity / 100000000;

  OrderCancellation rsp{};
  rsp.order_id = std::stoul(msg.client_order_id);
  rsp.canceled_volume = total - traded;
  oms_->OnOrderCanceled(&rsp);
}

void BssBroker::OnOrderCancelRejected(const bss::ExecutionReport &msg) {
  spdlog::error("[BssBroker::OnOrderCancelRejected] RejectCode:{} Reason:{}",
                msg.cancel_reject_code, msg.reason.data);
  OrderCancelRejection rsp{};
  rsp.order_id = std::stoul(msg.original_client_order_id);
  oms_->OnOrderCancelRejected(&rsp);
}

// 暂未支持
void BssBroker::on_order_amended(const bss::ExecutionReport &msg) {
  int qty = msg.execution_quantity / 100000000;
  double price = static_cast<double>(msg.execution_price) / 1e8;
  spdlog::debug(
      "[BssBroker::on_order_amended] ClientOrderID:{} OriginalOrderID:{} "
      "Qty:{} Price:{}",
      msg.client_order_id, msg.original_client_order_id, qty, price);

  uint32_t client_order_id = atoi(msg.original_client_order_id);
}

// 暂未支持
void BssBroker::on_order_amend_rejected(const bss::ExecutionReport &msg) {
  spdlog::error("[BssBroker::on_order_amend_rejected] RejectCode:{} Reason:{}",
                msg.amend_reject_code, msg.reason.data);
  uint32_t client_order_id = atoi(msg.original_client_order_id);
}

void BssBroker::on_order_expired(const bss::ExecutionReport &msg) {
  spdlog::error("[BssBroker::on_order_expired] RejectReason:{} Reason:{}", msg.order_reject_code,
                msg.reason.data);

  OrderRejection rsp{};
  rsp.order_id = std::stoul(msg.client_order_id);
  oms_->OnOrderRejected(&rsp);
}

REGISTER_GATEWAY(::ft::BssBroker);

}  // namespace ft
