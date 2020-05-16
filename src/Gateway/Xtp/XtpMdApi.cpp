// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "Gateway/Xtp/XtpMdApi.h"

#include <spdlog/spdlog.h>

namespace ft {

XtpMdApi::XtpMdApi(TradingEngineInterface* engine) : engine_(engine) {}

bool XtpMdApi::login(const LoginParams& params) {
  if (is_logon_) {
    spdlog::error("[XtpMdApi::login] Don't login twice");
    return false;
  }

  uint32_t seed = time(nullptr);
  uint8_t client_id = rand_r(&seed) & 0xff;
  quote_api_.reset(XTP::API::QuoteApi::CreateQuoteApi(client_id, "."));
  if (!quote_api_) {
    spdlog::error("[XtpMdApi::login] Failed to CreateQuoteApi");
    return false;
  }

  char protocol[32]{};
  char ip[32]{};
  int port = 0;

  try {
    int ret = sscanf(params.md_server_addr().c_str(), "%[^:]://%[^:]:%d",
                     protocol, ip, &port);
    assert(ret == 3);
  } catch (...) {
    assert(false);
  }

  XTP_PROTOCOL_TYPE sock_type = XTP_PROTOCOL_TCP;
  if (strcmp(protocol, "udp") == 0) sock_type = XTP_PROTOCOL_UDP;

  if (quote_api_->Login(ip, port, params.investor_id().c_str(),
                        params.passwd().c_str(), sock_type) != 0) {
    spdlog::error("[XtpMdApi::login] Failed to login: {}",
                  quote_api_->GetApiLastError()->error_msg);
    return false;
  }

  is_logon_ = true;
  return true;
}

void XtpMdApi::logout() {
  if (is_logon_) {
    quote_api_->Logout();
    is_logon_ = false;
  }
}

bool XtpMdApi::query_contract(const std::string& ticker) { return true; }

bool XtpMdApi::query_contracts() { return query_contract(""); }

}  // namespace ft
