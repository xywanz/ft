// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_SRC_GATEWAY_XTP_XTPMDAPI_H_
#define FT_SRC_GATEWAY_XTP_XTPMDAPI_H_

#include <xtp_quote_api.h>

#include <atomic>
#include <map>
#include <memory>
#include <mutex>
#include <string>

#include "Core/LoginParams.h"
#include "Core/Protocol.h"
#include "Core/TradingEngineInterface.h"
#include "Gateway/Xtp/XtpCommon.h"

namespace ft {

class XtpMdApi : public XTP::API::QuoteSpi {
 public:
  explicit XtpMdApi(TradingEngineInterface* engine);

  bool login(const LoginParams& params);

  void logout();

  bool query_contract(const std::string& ticker);

  bool query_contracts();

  void OnQueryAllTickers(XTPQSI* ticker_info, XTPRI* error_info,
                         bool is_last) override;

 private:
  void done() { is_done_ = true; }

  void error() { is_error_ = true; }

  void reset_sync() { is_done_ = false; }

  bool wait_sync() {
    while (!is_done_)
      if (is_error_) return false;

    return true;
  }

 private:
  TradingEngineInterface* engine_;
  std::unique_ptr<XTP::API::QuoteApi, XtpApiDeleter> quote_api_;

  volatile bool is_logon_ = false;
  volatile bool is_error_ = false;
  volatile bool is_done_ = false;

  std::mutex query_mutex_;
};

}  // namespace ft

#endif  // FT_SRC_GATEWAY_XTP_XTPMDAPI_H_
