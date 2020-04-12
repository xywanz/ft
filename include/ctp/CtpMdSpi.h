// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_INCLUDE_CTP_CTPMDSPI_H_
#define FT_INCLUDE_CTP_CTPMDSPI_H_

#include <ThostFtdcMdApi.h>

#include "ctp/CtpMdReceiver.h"

namespace ft {

class CtpMdSpi : public CThostFtdcMdSpi {
 public:
  explicit CtpMdSpi(CtpMdReceiver* receiver)
    : receiver_(receiver) {
  }

  void OnFrontConnected() override {
    receiver_->on_connected();
  }

  void OnFrontDisconnected(int nReason) override {
    receiver_->on_disconnected(nReason);
  }

  void OnHeartBeatWarning(int nTimeLapse) override {
    receiver_->on_heart_beat_warning(nTimeLapse);
  }

  void OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin,
                      CThostFtdcRspInfoField *pRspInfo,
                      int nRequestID,
                      bool bIsLast) override {
    receiver_->on_login(pRspUserLogin, pRspInfo, nRequestID, bIsLast);
  }

  void OnRspUserLogout(CThostFtdcUserLogoutField *pUserLogout,
                       CThostFtdcRspInfoField *pRspInfo,
                       int nRequestID,
                       bool bIsLast) override {
    receiver_->on_logout(pUserLogout, pRspInfo, nRequestID, bIsLast);
  }

  void OnRspError(CThostFtdcRspInfoField *pRspInfo,
                  int nRequestID,
                  bool bIsLast) override {
    receiver_->on_error(pRspInfo, nRequestID, bIsLast);
  }

  void OnRspSubMarketData(CThostFtdcSpecificInstrumentField *pSpecificInstrument,
                          CThostFtdcRspInfoField *pRspInfo,
                          int nRequestID,
                          bool bIsLast) override {
    receiver_->on_sub_md(pSpecificInstrument, pRspInfo, nRequestID, bIsLast);
  }

  void OnRspUnSubMarketData(CThostFtdcSpecificInstrumentField *pSpecificInstrument,
                            CThostFtdcRspInfoField *pRspInfo,
                            int nRequestID,
                            bool bIsLast) override {
    receiver_->on_unsub_md(pSpecificInstrument, pRspInfo, nRequestID, bIsLast);
  }

  void OnRspSubForQuoteRsp(CThostFtdcSpecificInstrumentField *pSpecificInstrument,
                           CThostFtdcRspInfoField *pRspInfo,
                           int nRequestID,
                           bool bIsLast) override {
    receiver_->on_sub_for_quote_rsp(pSpecificInstrument, pRspInfo, nRequestID, bIsLast);
  }

  void OnRspUnSubForQuoteRsp(CThostFtdcSpecificInstrumentField *pSpecificInstrument,
                             CThostFtdcRspInfoField *pRspInfo,
                             int nRequestID,
                             bool bIsLast) override {
    receiver_->on_unsub_for_quote_rsp(pSpecificInstrument, pRspInfo, nRequestID, bIsLast);
  }

  void OnRtnDepthMarketData(CThostFtdcDepthMarketDataField *pDepthMarketData) override {
    receiver_->on_depth_md(pDepthMarketData);
  }

  void OnRtnForQuoteRsp(CThostFtdcForQuoteRspField *pForQuoteRsp) override {
    receiver_->on_for_quote_rsp(pForQuoteRsp);
  }

 private:
  CtpMdReceiver* receiver_;
};

}  // namespace ft

#endif  // FT_INCLUDE_CTP_CTPMDSPI_H_
