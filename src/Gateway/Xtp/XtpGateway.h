// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_SRC_GATEWAY_XTP_XTPGATEWAY_H_
#define FT_SRC_GATEWAY_XTP_XTPGATEWAY_H_

#include <xtp_quote_api.h>
#include <xtp_trader_api.h>

#include <memory>
#include <string>

#include "Core/Gateway.h"
#include "Gateway/Xtp/XtpCommon.h"
#include "Gateway/Xtp/XtpMdApi.h"
#include "Gateway/Xtp/XtpTradeApi.h"

namespace ft {

class XtpGateway : public Gateway {
 public:
  explicit XtpGateway(TradingEngineInterface* engine);

  bool login(const LoginParams& params) override;

  void logout() override;

  bool query_contract(const std::string& ticker) override;

  bool query_contracts() override;

  bool query_account() override;

  bool query_position(const std::string& ticker) override;

  bool query_positions() override;

 private:
  TradingEngineInterface* engine_ = nullptr;
  std::unique_ptr<XtpTradeApi> trade_api_;
  std::unique_ptr<XtpMdApi> md_api_;
};

}  // namespace ft

#endif  // FT_SRC_GATEWAY_XTP_XTPGATEWAY_H_
