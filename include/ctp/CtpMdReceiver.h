// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_INCLUDE_CTP_CTPMDRECEIVER_H_
#define FT_INCLUDE_CTP_CTPMDRECEIVER_H_

#include <atomic>
#include <limits>
#include <map>
#include <string>
#include <vector>

#include <ThostFtdcMdApi.h>

#include "Common.h"
#include "LoginParams.h"
#include "MdReceiverInterface.h"
#include "Trader.h"
#include "TraderInterface.h"

namespace ft {

template<class PriceType>
inline PriceType adjust_price(PriceType price) {
  PriceType ret = price;
  if (price >= std::numeric_limits<PriceType>::max() - PriceType(1e-6))
    ret = PriceType(0);
  return ret;
}

class CtpMdReceiver : public MdReceiverInterface {
 public:
  CtpMdReceiver();

  void register_cb(TraderInterface* trader) override {
    trader_ = trader;
  }

  // need front_addr, broker_id, investor_id and passwd
  bool login(const LoginParams& params) override;

  void join() override {
    if (is_login_)
      api_->Join();
  }

  void on_connected();

  void on_disconnected(int reason);

  void on_heart_beat_warning(int time_lapse);

  void on_login(CThostFtdcRspUserLoginField *rsp_user_login,
                CThostFtdcRspInfoField *rsp_info,
                int req_id,
                bool is_last);

  void on_logout(CThostFtdcUserLogoutField *uesr_logout,
                 CThostFtdcRspInfoField *rsp_info,
                 int req_id,
                 bool is_last);

  void on_error(CThostFtdcRspInfoField *rsp_info,
                int req_id,
                bool is_last);

  void on_sub_md(CThostFtdcSpecificInstrumentField *specific_instrument,
                 CThostFtdcRspInfoField *rsp_info,
                 int req_id,
                 bool is_last);

  void on_unsub_md(CThostFtdcSpecificInstrumentField *specific_instrument,
                   CThostFtdcRspInfoField *rsp_info,
                   int req_id,
                   bool is_last);

  void on_sub_for_quote_rsp(CThostFtdcSpecificInstrumentField *specific_instrument,
                            CThostFtdcRspInfoField *rsp_info,
                            int req_id,
                            bool is_last);

  void on_unsub_for_quote_rsp(CThostFtdcSpecificInstrumentField *specific_instrument,
                              CThostFtdcRspInfoField *rsp_info,
                              int req_id,
                              bool is_last);

  void on_depth_md(CThostFtdcDepthMarketDataField *depth_market_data);

  void on_for_quote_rsp(CThostFtdcForQuoteRspField *for_quote_rsp);

 private:
  AsyncStatus req_async_status(int req_id) {
    auto res = req_status_.emplace(req_id, AsyncStatus{true});
    return res.first->second;
  }

  void rsp_async_status(int req_id, bool success) {
    auto status = req_status_[req_id];
    req_status_.erase(req_id);
    if (success)
      status.set_success();
    else
      status.set_error();
  }

  int next_req_id() {
    return next_req_id_++;
  }

 private:
  TraderInterface* trader_;

  CThostFtdcMdSpi* spi_ = nullptr;
  CThostFtdcMdApi* api_ = nullptr;

  std::string front_addr_;
  std::string investor_id_;

  int next_req_id_ = 0;

  std::atomic<bool> is_connected_ = false;
  std::atomic<bool> is_login_ = false;
  std::map<int, AsyncStatus> req_status_;

  std::vector<std::string> subscribed_list_;
};

}  // namespace ft

#endif  // FT_INCLUDE_CTP_CTPMDRECEIVER_H_
