// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_SRC_API_CTP_CTPMDAPI_H_
#define FT_SRC_API_CTP_CTPMDAPI_H_

#include <ThostFtdcMdApi.h>

namespace ft {

class CtpGateway;

class CtpMdSpi : public CThostFtdcMdSpi {
 public:
  explicit CtpMdSpi(CtpGateway *gateway);

  void OnFrontConnected();
  void OnFrontDisconnected(int reason) override;
  void OnHeartBeatWarning(int time_lapse) override;
  void OnRspUserLogin(CThostFtdcRspUserLoginField *login_rsp,
                      CThostFtdcRspInfoField *rsp_info, int req_id,
                      bool is_last) override;
  void OnRspUserLogout(CThostFtdcUserLogoutField *logout_rsp,
                       CThostFtdcRspInfoField *rsp_info, int req_id,
                       bool is_last) override;
  void OnRspError(CThostFtdcRspInfoField *rsp_info, int req_id,
                  bool is_last) override;
  void OnRspSubMarketData(CThostFtdcSpecificInstrumentField *instrument,
                          CThostFtdcRspInfoField *rsp_info, int req_id,
                          bool is_last) override;
  void OnRspUnSubMarketData(CThostFtdcSpecificInstrumentField *instrument,
                            CThostFtdcRspInfoField *rsp_info, int req_id,
                            bool is_last) override;
  void OnRspSubForQuoteRsp(CThostFtdcSpecificInstrumentField *instrument,
                           CThostFtdcRspInfoField *rsp_info, int req_id,
                           bool is_last) override;
  void OnRspUnSubForQuoteRsp(CThostFtdcSpecificInstrumentField *instrument,
                             CThostFtdcRspInfoField *rsp_info, int req_id,
                             bool is_last) override;
  void OnRtnDepthMarketData(CThostFtdcDepthMarketDataField *md) override;
  void OnRtnForQuoteRsp(CThostFtdcForQuoteRspField *for_quote_rsp) override;

 private:
  CtpGateway *gateway_ = nullptr;
};

}  // namespace ft

#endif  // FT_SRC_API_CTP_CTPMDAPI_H_
