// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_SRC_GATEWAY_XTP_XTPTRADEAPI_H_
#define FT_SRC_GATEWAY_XTP_XTPTRADEAPI_H_

#include <xtp_trader_api.h>

#include <memory>

#include "Core/TradingEngineInterface.h"
#include "Gateway/Xtp/XtpCommon.h"

namespace ft {

class XtpTradeApi : public XTP::API::TraderSpi {
 public:
  explicit XtpTradeApi(TradingEngineInterface* engine);

 private:
  TradingEngineInterface* engine_;
  std::unique_ptr<XTP::API::TraderApi, XtpApiDeleter> trade_api_;
};

}  // namespace ft

#endif  // FT_SRC_GATEWAY_XTP_XTPTRADEAPI_H_
