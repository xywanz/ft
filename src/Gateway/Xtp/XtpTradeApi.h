// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_SRC_GATEWAY_XTP_XTPTRADEAPI_H_
#define FT_SRC_GATEWAY_XTP_XTPTRADEAPI_H_

#include <xtp_trader_api.h>

#include <atomic>
#include <map>
#include <memory>
#include <mutex>
#include <string>

#include "Core/Config.h"
#include "Core/Position.h"
#include "Core/Protocol.h"
#include "Core/TradingEngineInterface.h"
#include "Gateway/Xtp/XtpCommon.h"

namespace ft {

class XtpTradeApi : public XTP::API::TraderSpi {
 public:
  explicit XtpTradeApi(TradingEngineInterface* engine);

  ~XtpTradeApi();

  bool login(const Config& config);

  void logout();

  uint64_t send_order(const OrderReq* order);

  bool cancel_order(uint64_t order_id);

  bool query_position(const std::string& ticker);

  bool query_positions();

  bool query_account();

  bool query_orders();

  bool query_trades();

  void OnOrderEvent(XTPOrderInfo* order_info, XTPRI* error_info,
                    uint64_t session_id) override;

  void OnTradeEvent(XTPTradeReport* trade_info, uint64_t session_id) override;

  void OnCancelOrderError(XTPOrderCancelInfo* cancel_info, XTPRI* error_info,
                          uint64_t session_id) override;

  void OnQueryPosition(XTPQueryStkPositionRsp* position, XTPRI* error_info,
                       int request_id, bool is_last,
                       uint64_t session_id) override;

  void OnQueryAsset(XTPQueryAssetRsp* asset, XTPRI* error_info, int request_id,
                    bool is_last, uint64_t session_id) override;

  void OnQueryOrder(XTPQueryOrderRsp* order_info, XTPRI* error_info,
                    int request_id, bool is_last, uint64_t session_id) override;

  void OnQueryTrade(XTPQueryTradeRsp* trade_info, XTPRI* error_info,
                    int request_id, bool is_last, uint64_t session_id) override;

 private:
  uint32_t next_client_order_id() { return next_client_order_id_++; }

  int next_req_id() { return next_req_id_++; }

  void done() { is_done_ = true; }

  void error() { is_error_ = true; }

  bool wait_sync() {
    while (!is_done_)
      if (is_error_) return false;

    is_done_ = false;
    return true;
  }

 private:
  struct OrderDetail {
    const Contract* contract = nullptr;
    bool accepted_ack = false;
    int original_vol = 0;
    int traded_vol = 0;
    int canceled_vol = 0;
  };

  TradingEngineInterface* engine_;
  std::unique_ptr<XTP::API::TraderApi, XtpApiDeleter> trade_api_;

  uint64_t session_id_ = 0;
  std::atomic<uint32_t> next_client_order_id_ = 1;
  std::atomic<uint32_t> next_req_id_ = 1;

  std::map<uint64_t, OrderDetail> order_details_;
  std::mutex order_mutex_;

  volatile bool is_done_ = false;
  volatile bool is_error_ = false;
  std::mutex query_mutex_;

  std::map<uint64_t, Position> pos_cache_;
};

}  // namespace ft

#endif  // FT_SRC_GATEWAY_XTP_XTPTRADEAPI_H_
