// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_INCLUDE_CTP_CTPTRADESPI_H_
#define FT_INCLUDE_CTP_CTPTRADESPI_H_

#include <ThostFtdcTraderApi.h>

#include "ctp/CtpGateway.h"

namespace ft {

class CtpTradeSpi : public CThostFtdcTraderSpi {
 public:
  explicit CtpTradeSpi(CtpGateway* gateway)
    : gateway_(gateway) {
  }

  // 当客户端与交易后台建立起通信连接时（还未登录前），该方法被调用。
  void OnFrontConnected() override {
    gateway_->on_connected();
  }

  // 当客户端与交易后台通信连接断开时，该方法被调用。
  // 当发生这个情况后，API会自动重新连接，客户端可不做处理。
  // @param nReason 错误原因
  //         0x1001 网络读失败
  //         0x1002 网络写失败
  //         0x2001 接收心跳超时
  //        0x2002  发送心跳失败
  //        0x2003  收到错误报文
  void OnFrontDisconnected(int nReason) override {
    gateway_->on_disconnected(nReason);
  }

  // 心跳超时警告。当长时间未收到报文时，该方法被调用。
  // @param nTimeLapse 距离上次接收报文的时间
  void OnHeartBeatWarning(int nTimeLapse) override {
    gateway_->on_heart_beat_warning(nTimeLapse);
  }

  // 客户端认证响应
  void OnRspAuthenticate(CThostFtdcRspAuthenticateField *pRspAuthenticateField,
                         CThostFtdcRspInfoField *pRspInfo,
                         int nRequestID,
                         bool bIsLast) override {
    gateway_->on_authenticate(pRspAuthenticateField, pRspInfo, nRequestID, bIsLast);
  }

  // 登录请求响应
  void OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin,
                      CThostFtdcRspInfoField *pRspInfo,
                      int nRequestID,
                      bool bIsLast) override {
    gateway_->on_login(pRspUserLogin, pRspInfo, nRequestID, bIsLast);
  }

  void OnRspQrySettlementInfo(CThostFtdcSettlementInfoField *pSettlementInfo,
                              CThostFtdcRspInfoField *pRspInfo,
                              int nRequestID,
                              bool bIsLast) {
  gateway_->on_settlement(pSettlementInfo, pRspInfo, nRequestID, bIsLast);
}

  void OnRspSettlementInfoConfirm(
          CThostFtdcSettlementInfoConfirmField *pSettlementInfoConfirm,
          CThostFtdcRspInfoField *pRspInfo,
          int nRequestID,
          bool bIsLast) override {
    gateway_->on_settlement_confirm(pSettlementInfoConfirm, pRspInfo,
                                    nRequestID, bIsLast);
  }

  void OnRspOrderInsert(CThostFtdcInputOrderField *pInputOrder,
                        CThostFtdcRspInfoField *pRspInfo,
                        int nRequestID,
                        bool bIsLast) override {
    gateway_->on_order_rejected(pInputOrder, pRspInfo, nRequestID, bIsLast);
  }

  void OnRspOrderAction(CThostFtdcInputOrderActionField *pInputOrderAction,
                        CThostFtdcRspInfoField *pRspInfo,
                        int nRequestID,
                        bool bIsLast) override {
    gateway_->on_order_action(pInputOrderAction, pRspInfo, nRequestID, bIsLast);
  }

  void OnRtnOrder(CThostFtdcOrderField *pOrder) {
    gateway_->on_order(pOrder);
  }

  void OnRtnTrade(CThostFtdcTradeField *pTrade) override {
    gateway_->on_trade(pTrade);
  }

  void OnRspQryInstrument(CThostFtdcInstrumentField *pInstrument,
                          CThostFtdcRspInfoField *pRspInfo,
                          int nRequestID,
                          bool bIsLast) override {
    gateway_->on_contract(pInstrument, pRspInfo, nRequestID, bIsLast);
  }

  void OnRspQryInvestorPosition(
          CThostFtdcInvestorPositionField *pInvestorPosition,
          CThostFtdcRspInfoField *pRspInfo,
          int nRequestID,
          bool bIsLast) override {
    gateway_->on_position(pInvestorPosition, pRspInfo, nRequestID, bIsLast);
  }

  void OnRspQryTradingAccount(CThostFtdcTradingAccountField *pTradingAccount,
                              CThostFtdcRspInfoField *pRspInfo,
                              int nRequestID,
                              bool bIsLast) override {
    gateway_->on_account(pTradingAccount, pRspInfo, nRequestID, bIsLast);
  }

 private:
  CtpGateway* gateway_;
};

}  // namespace ft

#endif  // FT_INCLUDE_CTP_CTPTRADESPI_H_
