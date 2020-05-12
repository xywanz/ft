// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_SRC_API_CTP_CTPGATEWAY_H_
#define FT_SRC_API_CTP_CTPGATEWAY_H_

#include <ThostFtdcMdApi.h>
#include <ThostFtdcTraderApi.h>

#include <atomic>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "Core/Gateway.h"
#include "Ctp/CtpCommon.h"
#include "Ctp/CtpMdSpi.h"
#include "Ctp/CtpTradeSpi.h"

namespace ft {

class CtpGateway : public Gateway {
 public:
  explicit CtpGateway(TradingEngineInterface *engine);

  ~CtpGateway();

  bool login(const LoginParams &params);

  void logout();

  uint64_t send_order(const OrderReq *order);

  bool cancel_order(uint64_t order_id);

  bool query_contract(const std::string &ticker) override;

  bool query_contracts() override;

  bool query_position(const std::string &ticker);

  bool query_positions() override;

  bool query_account() override;

  bool query_orders();

  bool query_trades();

  bool query_margin_rate(const std::string &ticker) override;

  void OnFrontConnected();
  void OnFrontDisconnected(int reason);
  void OnHeartBeatWarning(int time_lapse);
  void OnRspAuthenticate(CThostFtdcRspAuthenticateField *auth_rsp,
                         CThostFtdcRspInfoField *rsp_info, int req_id,
                         bool is_last);
  void OnRspUserLogin(CThostFtdcRspUserLoginField *rsp_user_login,
                      CThostFtdcRspInfoField *rsp_info, int req_id,
                      bool is_last);
  void OnRspQrySettlementInfo(CThostFtdcSettlementInfoField *settlement,
                              CThostFtdcRspInfoField *rsp_info, int req_id,
                              bool is_last);
  void OnRspSettlementInfoConfirm(CThostFtdcSettlementInfoConfirmField *confirm,
                                  CThostFtdcRspInfoField *rsp_info, int req_id,
                                  bool is_last);
  void OnRspUserLogout(CThostFtdcUserLogoutField *user_logout,
                       CThostFtdcRspInfoField *rsp_info, int req_id,
                       bool is_last);
  void OnRspOrderInsert(CThostFtdcInputOrderField *ctp_order,
                        CThostFtdcRspInfoField *rsp_info, int req_id,
                        bool is_last);
  void OnRspOrderAction(CThostFtdcInputOrderActionField *action,
                        CThostFtdcRspInfoField *rsp_info, int req_id,
                        bool is_last);
  void OnRtnOrder(CThostFtdcOrderField *ctp_order);
  void OnRtnTrade(CThostFtdcTradeField *trade);
  void OnRspQryInstrument(CThostFtdcInstrumentField *instrument,
                          CThostFtdcRspInfoField *rsp_info, int req_id,
                          bool is_last);
  void OnRspQryInvestorPosition(CThostFtdcInvestorPositionField *position,
                                CThostFtdcRspInfoField *rsp_info, int req_id,
                                bool is_last);
  void OnRspQryTradingAccount(CThostFtdcTradingAccountField *trading_account,
                              CThostFtdcRspInfoField *rsp_info, int req_id,
                              bool is_last);
  void OnRspQryOrder(CThostFtdcOrderField *order,
                     CThostFtdcRspInfoField *rsp_info, int req_id,
                     bool is_last);
  void OnRspQryTrade(CThostFtdcTradeField *trade,
                     CThostFtdcRspInfoField *rsp_info, int req_id,
                     bool is_last);
  void OnRspQryInstrumentMarginRate(
      CThostFtdcInstrumentMarginRateField *margin_rate,
      CThostFtdcRspInfoField *rsp_info, int req_id, bool is_last);

  void OnFrontConnectedMD();
  void OnFrontDisconnectedMD(int reason);
  void OnHeartBeatWarningMD(int time_lapse);
  void OnRspUserLoginMD(CThostFtdcRspUserLoginField *login_rsp,
                        CThostFtdcRspInfoField *rsp_info, int req_id,
                        bool is_last);
  void OnRspUserLogoutMD(CThostFtdcUserLogoutField *logout_rsp,
                         CThostFtdcRspInfoField *rsp_info, int req_id,
                         bool is_last);

  void OnRspErrorMD(CThostFtdcRspInfoField *rsp_info, int req_id, bool is_last);
  void OnRspSubMarketData(CThostFtdcSpecificInstrumentField *instrument,
                          CThostFtdcRspInfoField *rsp_info, int req_id,
                          bool is_last);
  void OnRspUnSubMarketData(CThostFtdcSpecificInstrumentField *instrument,
                            CThostFtdcRspInfoField *rsp_info, int req_id,
                            bool is_last);
  void OnRspSubForQuoteRsp(CThostFtdcSpecificInstrumentField *instrument,
                           CThostFtdcRspInfoField *rsp_info, int req_id,
                           bool is_last);
  void OnRspUnSubForQuoteRsp(CThostFtdcSpecificInstrumentField *instrument,
                             CThostFtdcRspInfoField *rsp_info, int req_id,
                             bool is_last);
  void OnRtnDepthMarketData(CThostFtdcDepthMarketDataField *md);
  void OnRtnForQuoteRsp(CThostFtdcForQuoteRspField *for_quote_rsp);

 private:
  struct OrderDetail {
    const Contract *contract = nullptr;
    int order_sysid = 0;
    bool accepted_ack = false;
    int64_t original_vol = 0;
    int64_t traded_vol = 0;
    int64_t canceled_vol = 0;
  };

  bool login_into_trading_server(const LoginParams &params);
  bool login_into_md_server(const LoginParams &params);

  static uint64_t get_order_id(uint64_t ticker_index, int order_ref) {
    return (ticker_index << 32) | static_cast<uint64_t>(order_ref);
  }

  int next_req_id() { return next_req_id_++; }

  int next_order_ref() { return next_order_ref_++; }

  void done() { is_done_ = true; }

  void error() { is_error_ = true; }

  void reset_sync() { is_done_ = false; }

  bool wait_sync() {
    while (!is_done_)
      if (is_error_) return false;

    return true;
  }

 private:
  std::unique_ptr<CtpTradeSpi> trade_spi_;
  std::unique_ptr<CtpMdSpi> md_spi_;

  std::unique_ptr<CThostFtdcTraderApi, CtpApiDeleter> trade_api_;
  std::unique_ptr<CThostFtdcMdApi, CtpApiDeleter> md_api_;

  // trade
  std::string front_addr_;
  std::string broker_id_;
  std::string investor_id_;
  int front_id_ = 0;
  int session_id_ = 0;

  std::atomic<int> next_req_id_ = 0;
  std::atomic<int> next_order_ref_ = 0;

  std::atomic<bool> is_error_ = false;
  std::atomic<bool> is_connected_ = false;
  std::atomic<bool> is_done_ = false;
  std::atomic<bool> is_logon_ = false;

  std::map<uint64_t, Position> pos_cache_;
  std::map<uint64_t, OrderDetail> order_details_;
  std::mutex query_mutex_;
  std::mutex order_mutex_;

  // md
  std::atomic<bool> is_md_error_ = false;
  std::atomic<bool> is_md_connected_ = false;
  std::atomic<bool> is_md_login_ = false;

  std::vector<std::string> subscribed_list_;
  std::map<std::string, const Contract *> symbol2contract_;
};

}  // namespace ft

#endif  // FT_SRC_API_CTP_CTPGATEWAY_H_
