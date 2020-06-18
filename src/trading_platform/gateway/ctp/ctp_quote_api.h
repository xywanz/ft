// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_SRC_GATEWAY_CTP_CTP_QUOTE_API_H_
#define FT_SRC_GATEWAY_CTP_CTP_QUOTE_API_H_

#include <ThostFtdcMdApi.h>

#include <atomic>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "core/config.h"
#include "core/tick_data.h"
#include "gateway/ctp/ctp_common.h"
#include "interface/trading_engine_interface.h"

namespace ft {

class CtpQuoteApi : public CThostFtdcMdSpi {
 public:
  explicit CtpQuoteApi(TradingEngineInterface *engine);

  ~CtpQuoteApi();

  bool login(const Config &config);

  void logout();

  void OnFrontConnected() override;

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
  int next_req_id() { return next_req_id_++; }

 private:
  TradingEngineInterface *engine_;
  std::unique_ptr<CThostFtdcMdApi, CtpApiDeleter> quote_api_;

  std::string server_addr_;
  std::string broker_id_;
  std::string investor_id_;
  std::string passwd_;

  std::atomic<int> next_req_id_ = 0;

  std::atomic<bool> is_error_ = false;
  std::atomic<bool> is_connected_ = false;
  std::atomic<bool> is_logon_ = false;

  std::vector<std::string> sub_list_;
};

}  // namespace ft

#endif  // FT_SRC_GATEWAY_CTP_CTP_QUOTE_API_H_
