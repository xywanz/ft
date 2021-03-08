// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_SRC_GATEWAY_XTP__XTP_QUOTE_API_H_
#define FT_SRC_GATEWAY_XTP__XTP_QUOTE_API_H_

#include <atomic>
#include <map>
#include <mutex>
#include <string>
#include <vector>

#include "ft/trader/gateway.h"
#include "gateway/xtp/xtp_common.h"
#include "xtp_quote_api.h"

namespace ft {

class XtpQuoteApi : public XTP::API::QuoteSpi {
 public:
  explicit XtpQuoteApi(BaseOrderManagementSystem* oms);
  ~XtpQuoteApi();

  bool Login(const Config& config);
  void Logout();
  bool Subscribe(const std::vector<std::string>& sub_list);

  bool QueryContractList(std::vector<Contract>* result);

  void OnQueryAllTickers(XTPQSI* ticker_info, XTPRI* error_info, bool is_last) override;

  void OnDepthMarketData(XTPMD* market_data, int64_t bid1_qty[], int32_t bid1_count,
                         int32_t max_bid1_count, int64_t ask1_qty[], int32_t ask1_count,
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
  BaseOrderManagementSystem* oms_;
  XtpUniquePtr<XTP::API::QuoteApi> quote_api_;

  std::vector<std::string> subscribed_list_;
  std::vector<Contract>* contract_results_;

  volatile bool is_logon_ = false;
  volatile bool is_error_ = false;
  volatile bool is_done_ = false;
};

}  // namespace ft

#endif  // FT_SRC_GATEWAY_XTP__XTP_QUOTE_API_H_
