// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_SRC_GATEWAY_XTP_XTPTRADEAPI_H_
#define FT_SRC_GATEWAY_XTP_XTPTRADEAPI_H_

#include <xtp_trader_api.h>

#include <atomic>
#include <map>
#include <memory>
#include <mutex>

#include "Core/LoginParams.h"
#include "Core/Protocol.h"
#include "Core/TradingEngineInterface.h"
#include "Gateway/Xtp/XtpCommon.h"

namespace ft {

class XtpTradeApi : public XTP::API::TraderSpi {
 public:
  explicit XtpTradeApi(TradingEngineInterface* engine);

  bool login(const LoginParams& params);

  void logout();

  bool send_order(const OrderReq* order);

  bool cancel_order(uint64_t order_id);

  void OnOrderEvent(XTPOrderInfo* order_info, XTPRI* error_info,
                    uint64_t session_id) override;

  void OnTradeEvent(XTPTradeReport* trade_info, uint64_t session_id) override;

 private:
  uint32_t next_client_order_id() { return next_client_order_id_++; }

 private:
  struct OrderDetail {
    const Contract* contract = nullptr;
    uint64_t order_id = 0;
    bool accepted_ack = false;
    int64_t original_vol = 0;
    int64_t traded_vol = 0;
    int64_t canceled_vol = 0;
  };

  TradingEngineInterface* engine_;
  std::unique_ptr<XTP::API::TraderApi, XtpApiDeleter> trade_api_;

  uint64_t session_id_ = 0;
  std::atomic<uint32_t> next_client_order_id_ = 1;

  std::map<uint64_t, OrderDetail> order_details_;
  std::map<uint64_t, uint64_t> order_id_ft2xtp_;
  std::mutex order_mutex_;
};

}  // namespace ft

#endif  // FT_SRC_GATEWAY_XTP_XTPTRADEAPI_H_
