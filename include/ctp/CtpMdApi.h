// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_INCLUDE_CTP_CTPMDAPI_H_
#define FT_INCLUDE_CTP_CTPMDAPI_H_

#include <atomic>
#include <limits>
#include <map>
#include <string>
#include <vector>

#include <ThostFtdcMdApi.h>

#include "Common.h"
#include "GeneralApi.h"
#include "LoginParams.h"

namespace ft {

class CtpMdApi : public CThostFtdcMdSpi {
 public:
  explicit CtpMdApi(GeneralApi* general_api);

  ~CtpMdApi();

  // need front_addr, broker_id, investor_id and passwd
  bool login(const LoginParams& params);

  void OnFrontConnected();

  void OnFrontDisconnected(int reason) override;

  void OnHeartBeatWarning(int time_lapse) override;

  void OnRspUserLogin(CThostFtdcRspUserLoginField *rsp_user_login,
                      CThostFtdcRspInfoField *rsp_info,
                      int req_id,
                      bool is_last) override;

  void OnRspUserLogout(CThostFtdcUserLogoutField *uesr_logout,
                       CThostFtdcRspInfoField *rsp_info,
                       int req_id,
                       bool is_last) override;

  void OnRspError(CThostFtdcRspInfoField *rsp_info,
                  int req_id,
                  bool is_last) override;

  void OnRspSubMarketData(CThostFtdcSpecificInstrumentField *specific_instrument,
                          CThostFtdcRspInfoField *rsp_info,
                          int req_id,
                          bool is_last) override;

  void OnRspUnSubMarketData(CThostFtdcSpecificInstrumentField *specific_instrument,
                            CThostFtdcRspInfoField *rsp_info,
                            int req_id,
                            bool is_last) override;

  void OnRspSubForQuoteRsp(CThostFtdcSpecificInstrumentField *specific_instrument,
                           CThostFtdcRspInfoField *rsp_info,
                           int req_id,
                           bool is_last) override;

  void OnRspUnSubForQuoteRsp(CThostFtdcSpecificInstrumentField *specific_instrument,
                             CThostFtdcRspInfoField *rsp_info,
                             int req_id,
                             bool is_last) override;

  void OnRtnDepthMarketData(CThostFtdcDepthMarketDataField *depth_market_data) override;

  void OnRtnForQuoteRsp(CThostFtdcForQuoteRspField *for_quote_rsp) override;

 private:
  int next_req_id() {
    return next_req_id_++;
  }

 private:
  GeneralApi* general_api_ = nullptr;
  CThostFtdcMdApi* ctp_api_ = nullptr;

  std::string front_addr_;
  std::string investor_id_;

  int next_req_id_ = 0;

  std::atomic<bool> is_error_ = false;
  std::atomic<bool> is_connected_ = false;
  std::atomic<bool> is_login_ = false;

  std::vector<std::string> subscribed_list_;

  std::map<std::string, std::string> symbol2exchange_;
};


template<class PriceType>
inline PriceType adjust_price(PriceType price) {
  PriceType ret = price;
  if (price >= std::numeric_limits<PriceType>::max() - PriceType(1e-6))
    ret = PriceType(0);
  return ret;
}

}  // namespace ft

#endif  // FT_INCLUDE_CTP_CTPMDAPI_H_
