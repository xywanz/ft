// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_SRC_GATEWAY_XTP_XTPCOMMON_H_
#define FT_SRC_GATEWAY_XTP_XTPCOMMON_H_

#include <xtp_trader_api.h>

namespace ft {

struct XtpApiDeleter {
  template <class T>
  void operator()(T* p) {
    if (p) {
      p->RegisterSpi(nullptr);
      p->Release();
    }
  }
};

}  // namespace ft

#endif  // FT_SRC_GATEWAY_XTP_XTPCOMMON_H_
