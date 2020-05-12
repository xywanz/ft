// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "Ctp/CtpMdSpi.h"

#include "Ctp/CtpGateway.h"

namespace ft {

CtpMdSpi::CtpMdSpi(CtpGateway *gateway) : gateway_(gateway) {}

void CtpMdSpi::OnFrontConnected() { gateway_->OnFrontConnectedMD(); }

void CtpMdSpi::OnFrontDisconnected(int reason) {
  gateway_->OnFrontDisconnectedMD(reason);
}

void CtpMdSpi::OnHeartBeatWarning(int time_lapse) {
  gateway_->OnHeartBeatWarningMD(time_lapse);
}

void CtpMdSpi::OnRspUserLogin(CThostFtdcRspUserLoginField *login_rsp,
                              CThostFtdcRspInfoField *rsp_info, int req_id,
                              bool is_last) {
  gateway_->OnRspUserLoginMD(login_rsp, rsp_info, req_id, is_last);
}

void CtpMdSpi::OnRspUserLogout(CThostFtdcUserLogoutField *logout_rsp,
                               CThostFtdcRspInfoField *rsp_info, int req_id,
                               bool is_last) {
  gateway_->OnRspUserLogoutMD(logout_rsp, rsp_info, req_id, is_last);
}

void CtpMdSpi::OnRspError(CThostFtdcRspInfoField *rsp_info, int req_id,
                          bool is_last) {
  gateway_->OnRspErrorMD(rsp_info, req_id, is_last);
}

void CtpMdSpi::OnRspSubMarketData(CThostFtdcSpecificInstrumentField *instrument,
                                  CThostFtdcRspInfoField *rsp_info, int req_id,
                                  bool is_last) {
  gateway_->OnRspSubMarketData(instrument, rsp_info, req_id, is_last);
}

void CtpMdSpi::OnRspUnSubMarketData(
    CThostFtdcSpecificInstrumentField *instrument,
    CThostFtdcRspInfoField *rsp_info, int req_id, bool is_last) {
  gateway_->OnRspUnSubMarketData(instrument, rsp_info, req_id, is_last);
}

void CtpMdSpi::OnRspSubForQuoteRsp(
    CThostFtdcSpecificInstrumentField *instrument,
    CThostFtdcRspInfoField *rsp_info, int req_id, bool is_last) {
  gateway_->OnRspSubForQuoteRsp(instrument, rsp_info, req_id, is_last);
}

void CtpMdSpi::OnRspUnSubForQuoteRsp(
    CThostFtdcSpecificInstrumentField *instrument,
    CThostFtdcRspInfoField *rsp_info, int req_id, bool is_last) {
  gateway_->OnRspUnSubForQuoteRsp(instrument, rsp_info, req_id, is_last);
}

void CtpMdSpi::OnRtnDepthMarketData(CThostFtdcDepthMarketDataField *md) {
  gateway_->OnRtnDepthMarketData(md);
}

void CtpMdSpi::OnRtnForQuoteRsp(CThostFtdcForQuoteRspField *for_quote_rsp) {
  gateway_->OnRtnForQuoteRsp(for_quote_rsp);
}

}  // namespace ft
