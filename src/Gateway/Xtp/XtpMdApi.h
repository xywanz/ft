// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_SRC_GATEWAY_XTP_XTPMDAPI_H_
#define FT_SRC_GATEWAY_XTP_XTPMDAPI_H_

#include <xtp_quote_api.h>

#include <atomic>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "Core/LoginParams.h"
#include "Core/Protocol.h"
#include "Core/TradingEngineInterface.h"
#include "Gateway/Xtp/XtpCommon.h"

namespace ft {

class XtpMdApi : public XTP::API::QuoteSpi {
 public:
  explicit XtpMdApi(TradingEngineInterface* engine);

  ~XtpMdApi();

  bool login(const LoginParams& params);

  void logout();

  bool query_contract(const std::string& ticker, const std::string& exchange);

  bool query_contracts();

  void OnQueryAllTickers(XTPQSI* ticker_info, XTPRI* error_info,
                         bool is_last) override;

  void OnDepthMarketData(XTPMD* market_data, int64_t bid1_qty[],
                         int32_t bid1_count, int32_t max_bid1_count,
                         int64_t ask1_qty[], int32_t ask1_count,
                         int32_t max_ask1_count) override;

 private:
  void done() { is_done_ = true; }

  void error() { is_error_ = true; }

  bool wait_sync() {
    while (!is_done_)
      if (is_error_) return false;

    is_done_ = false;
    return true;
  }

 private:
  TradingEngineInterface* engine_;
  std::unique_ptr<XTP::API::QuoteApi, XtpApiDeleter> quote_api_;

  std::vector<std::string> subscribed_list_;

  volatile bool is_logon_ = false;
  volatile bool is_error_ = false;
  volatile bool is_done_ = false;

  std::mutex query_mutex_;
};

}  // namespace ft

#endif  // FT_SRC_GATEWAY_XTP_XTPMDAPI_H_
