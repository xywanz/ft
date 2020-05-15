// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "Gateway/Xtp/XtpGateway.h"

namespace ft {

XtpGateway::XtpGateway(TradingEngineInterface* engine)
    : Gateway(engine), engine_(engine) {
  trade_api_.reset(new XtpTradeApi(engine));
}

bool XtpGateway::login(const LoginParams& params) {}

}  // namespace ft
