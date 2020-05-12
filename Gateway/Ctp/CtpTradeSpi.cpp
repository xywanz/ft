// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "Ctp/CtpTradeSpi.h"

#include <ThostFtdcTraderApi.h>

#include "Ctp/CtpGateway.h"

namespace ft {

CtpTradeSpi::CtpTradeSpi(CtpGateway *gateway) : gateway_(gateway) {}

void CtpTradeSpi::OnFrontConnected() { gateway_->OnFrontConnected(); }

void CtpTradeSpi::OnFrontDisconnected(int reason) {
  gateway_->OnFrontDisconnected(reason);
}

void CtpTradeSpi::OnHeartBeatWarning(int time_lapse) {
  gateway_->OnHeartBeatWarning(time_lapse);
}

void CtpTradeSpi::OnRspAuthenticate(CThostFtdcRspAuthenticateField *auth,
                                    CThostFtdcRspInfoField *rsp_info,
                                    int req_id, bool is_last) {
  gateway_->OnRspAuthenticate(auth, rsp_info, req_id, is_last);
}

void CtpTradeSpi::OnRspUserLogin(CThostFtdcRspUserLoginField *logon,
                                 CThostFtdcRspInfoField *rsp_info, int req_id,
                                 bool is_last) {
  gateway_->OnRspUserLogin(logon, rsp_info, req_id, is_last);
}

void CtpTradeSpi::OnRspQrySettlementInfo(
    CThostFtdcSettlementInfoField *settlement_info,
    CThostFtdcRspInfoField *rsp_info, int req_id, bool is_last) {
  gateway_->OnRspQrySettlementInfo(settlement_info, rsp_info, req_id, is_last);
}

void CtpTradeSpi::OnRspSettlementInfoConfirm(
    CThostFtdcSettlementInfoConfirmField *settlement_info_confirm,
    CThostFtdcRspInfoField *rsp_info, int req_id, bool is_last) {
  gateway_->OnRspSettlementInfoConfirm(settlement_info_confirm, rsp_info,
                                       req_id, is_last);
}

void CtpTradeSpi::OnRspUserLogout(CThostFtdcUserLogoutField *logout_field,
                                  CThostFtdcRspInfoField *rsp_info, int req_id,
                                  bool is_last) {
  gateway_->OnRspUserLogout(logout_field, rsp_info, req_id, is_last);
}

void CtpTradeSpi::OnRspOrderInsert(CThostFtdcInputOrderField *ctp_order,
                                   CThostFtdcRspInfoField *rsp_info, int req_id,
                                   bool is_last) {
  gateway_->OnRspOrderInsert(ctp_order, rsp_info, req_id, is_last);
}

void CtpTradeSpi::OnRtnOrder(CThostFtdcOrderField *ctp_order) {
  gateway_->OnRtnOrder(ctp_order);
}

void CtpTradeSpi::OnRtnTrade(CThostFtdcTradeField *trade) {
  gateway_->OnRtnTrade(trade);
}

void CtpTradeSpi::OnRspOrderAction(CThostFtdcInputOrderActionField *action,
                                   CThostFtdcRspInfoField *rsp_info, int req_id,
                                   bool is_last) {
  gateway_->OnRspOrderAction(action, rsp_info, req_id, is_last);
}

void CtpTradeSpi::OnRspQryInstrument(CThostFtdcInstrumentField *instrument,
                                     CThostFtdcRspInfoField *rsp_info,
                                     int req_id, bool is_last) {
  gateway_->OnRspQryInstrument(instrument, rsp_info, req_id, is_last);
}

void CtpTradeSpi::OnRspQryInvestorPosition(
    CThostFtdcInvestorPositionField *position, CThostFtdcRspInfoField *rsp_info,
    int req_id, bool is_last) {
  gateway_->OnRspQryInvestorPosition(position, rsp_info, req_id, is_last);
}

void CtpTradeSpi::OnRspQryTradingAccount(
    CThostFtdcTradingAccountField *trading_account,
    CThostFtdcRspInfoField *rsp_info, int req_id, bool is_last) {
  gateway_->OnRspQryTradingAccount(trading_account, rsp_info, req_id, is_last);
}

void CtpTradeSpi::OnRspQryOrder(CThostFtdcOrderField *order,
                                CThostFtdcRspInfoField *rsp_info, int req_id,
                                bool is_last) {
  gateway_->OnRspQryOrder(order, rsp_info, req_id, is_last);
}

void CtpTradeSpi::OnRspQryTrade(CThostFtdcTradeField *trade,
                                CThostFtdcRspInfoField *rsp_info, int req_id,
                                bool is_last) {
  gateway_->OnRspQryTrade(trade, rsp_info, req_id, is_last);
}

void CtpTradeSpi::OnRspQryInstrumentMarginRate(
    CThostFtdcInstrumentMarginRateField *margin_rate,
    CThostFtdcRspInfoField *rsp_info, int req_id, bool is_last) {
  gateway_->OnRspQryInstrumentMarginRate(margin_rate, rsp_info, req_id,
                                         is_last);
}

}  // namespace ft
