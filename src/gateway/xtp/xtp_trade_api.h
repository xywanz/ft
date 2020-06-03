// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_SRC_GATEWAY_XTP_XTP_TRADE_API_H_
#define FT_SRC_GATEWAY_XTP_XTP_TRADE_API_H_

#include <xtp_trader_api.h>

#include <atomic>
#include <map>
#include <memory>
#include <mutex>
#include <string>

#include "core/config.h"
#include "core/position.h"
#include "core/protocol.h"
#include "core/trading_engine_interface.h"
#include "gateway/xtp/xtp_common.h"

namespace ft {

class XtpTradeApi : public XTP::API::TraderSpi {
 public:
  explicit XtpTradeApi(TradingEngineInterface* engine);

  ~XtpTradeApi();

  bool login(const Config& config);

  void logout();

  bool send_order(const OrderReq& order);

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
  TradingEngineInterface* engine_;
  std::unique_ptr<XTP::API::TraderApi, XtpApiDeleter> trade_api_;

  std::string investor_id_;
  uint64_t session_id_ = 0;
  std::atomic<uint32_t> next_req_id_ = 1;

  volatile bool is_done_ = false;
  volatile bool is_error_ = false;

  std::map<uint64_t, Position> pos_cache_;
};

}  // namespace ft

#endif  // FT_SRC_GATEWAY_XTP_XTP_TRADE_API_H_
