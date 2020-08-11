// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_SRC_GATEWAY_XTP_XTP_TRADE_API_H_
#define FT_SRC_GATEWAY_XTP_XTP_TRADE_API_H_

#include <xtp_trader_api.h>

#include <atomic>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "cep/data/account.h"
#include "cep/data/config.h"
#include "cep/data/order.h"
#include "cep/data/position.h"
#include "cep/data/protocol.h"
#include "cep/data/response.h"
#include "cep/interface/oms_interface.h"
#include "xtp_common.h"

namespace ft {

class XtpTradeApi : public XTP::API::TraderSpi {
 public:
  explicit XtpTradeApi(OMSInterface* oms);
  ~XtpTradeApi();

  bool login(const Config& config);
  void logout();

  bool send_order(const OrderRequest& order, uint64_t* privdata_ptr);
  bool cancel_order(uint64_t xtp_order_id);

  bool query_positions(std::vector<Position>* result);
  bool query_account(Account* result);
  bool query_trades(std::vector<Trade>* result);
  bool query_orders();

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
  OMSInterface* oms_;
  XtpUniquePtr<XTP::API::TraderApi> trade_api_;

  std::string investor_id_;
  uint64_t session_id_ = 0;
  std::atomic<uint32_t> next_req_id_ = 1;

  volatile bool is_done_ = false;
  volatile bool is_error_ = false;

  Account* account_result_;
  std::vector<Position>* position_results_;
  std::vector<Trade>* trade_results_;
  std::map<uint64_t, Position> pos_cache_;
};

}  // namespace ft

#endif  // FT_SRC_GATEWAY_XTP_XTP_TRADE_API_H_
