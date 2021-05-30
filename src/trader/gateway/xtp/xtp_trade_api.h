// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_SRC_GATEWAY_XTP_XTP_TRADE_API_H_
#define FT_SRC_GATEWAY_XTP_XTP_TRADE_API_H_

#include <atomic>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "trader/gateway/gateway.h"
#include "trader/gateway/xtp/xtp_common.h"
#include "xtp_trader_api.h"

namespace ft {

class XtpGateway;

class XtpTradeApi : public XTP::API::TraderSpi {
 public:
  explicit XtpTradeApi(XtpGateway* gateway);
  ~XtpTradeApi();

  bool Login(const GatewayConfig& config);
  void Logout();

  bool SendOrder(const OrderRequest& order, uint64_t* privdata_ptr);
  bool CancelOrder(uint64_t xtp_order_id);

  bool QueryPositions();
  bool QueryAccount();
  bool QueryTrades();
  bool QueryOrders();

  void OnOrderEvent(XTPOrderInfo* order_info, XTPRI* error_info, uint64_t session_id) override;

  void OnTradeEvent(XTPTradeReport* trade_info, uint64_t session_id) override;

  void OnCancelOrderError(XTPOrderCancelInfo* cancel_info, XTPRI* error_info,
                          uint64_t session_id) override;

  void OnQueryPosition(XTPQueryStkPositionRsp* position, XTPRI* error_info, int request_id,
                       bool is_last, uint64_t session_id) override;

  void OnQueryAsset(XTPQueryAssetRsp* asset, XTPRI* error_info, int request_id, bool is_last,
                    uint64_t session_id) override;

  void OnQueryOrder(XTPQueryOrderRsp* order_info, XTPRI* error_info, int request_id, bool is_last,
                    uint64_t session_id) override;

  void OnQueryTrade(XTPQueryTradeRsp* trade_info, XTPRI* error_info, int request_id, bool is_last,
                    uint64_t session_id) override;

 private:
  int next_req_id() { return next_req_id_++; }

 private:
  XtpGateway* gateway_;
  XtpUniquePtr<XTP::API::TraderApi> trade_api_;

  std::string investor_id_;
  uint64_t session_id_ = 0;
  std::atomic<uint32_t> next_req_id_ = 1;
  std::atomic<int> status_ = 0;

  XtpDatetimeConverter dt_converter_;

  std::map<uint64_t, Position> pos_cache_;
};

}  // namespace ft

#endif  // FT_SRC_GATEWAY_XTP_XTP_TRADE_API_H_
