// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "Gateway/Xtp/XtpGateway.h"

namespace ft {

XtpGateway::XtpGateway(TradingEngineInterface* engine)
    : Gateway(engine),
      engine_(engine),
      trade_api_(std::make_unique<XtpTradeApi>(engine)),
      md_api_(std::make_unique<XtpMdApi>(engine)) {}

bool XtpGateway::login(const LoginParams& params) {
  return trade_api_->login(params);
}

void XtpGateway::logout() {
  trade_api_->logout();
  md_api_->logout();
}

}  // namespace ft
