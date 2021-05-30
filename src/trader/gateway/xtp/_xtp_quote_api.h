// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_SRC_GATEWAY_XTP__XTP_QUOTE_API_H_
#define FT_SRC_GATEWAY_XTP__XTP_QUOTE_API_H_

#include <atomic>
#include <map>
#include <mutex>
#include <string>
#include <vector>

#include "trader/gateway/gateway.h"
#include "trader/gateway/xtp/xtp_common.h"
#include "xtp_quote_api.h"

namespace ft {

class XtpGateway;

class XtpQuoteApi : public XTP::API::QuoteSpi {
 public:
  explicit XtpQuoteApi(XtpGateway* gateway);
  ~XtpQuoteApi();

  bool Login(const GatewayConfig& config);
  void Logout();
  bool Subscribe(const std::vector<std::string>& sub_list);

  bool QueryContracts();

  void OnQueryAllTickers(XTPQSI* ticker_info, XTPRI* error_info, bool is_last) override;

  void OnDepthMarketData(XTPMD* market_data, int64_t bid1_qty[], int32_t bid1_count,
                         int32_t max_bid1_count, int64_t ask1_qty[], int32_t ask1_count,
                         int32_t max_ask1_count) override;

 private:
  XtpGateway* gateway_;
  XtpUniquePtr<XTP::API::QuoteApi> quote_api_;
  std::vector<std::string> subscribed_list_;

  XtpDatetimeConverter dt_converter_;

  std::vector<Contract> qry_contract_res_;
  int query_count_ = 0;
};

}  // namespace ft

#endif  // FT_SRC_GATEWAY_XTP__XTP_QUOTE_API_H_
