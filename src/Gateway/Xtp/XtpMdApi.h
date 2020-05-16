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

 private:
  TradingEngineInterface* engine_;
  std::unique_ptr<XTP::API::QuoteApi, XtpApiDeleter> quote_api_;

  volatile bool is_logon_ = false;
};

}  // namespace ft

#endif  // FT_SRC_GATEWAY_XTP_XTPMDAPI_H_
