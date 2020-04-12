// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_INCLUDE_MDRECEIVERINTERFACE_H_
#define FT_INCLUDE_MDRECEIVERINTERFACE_H_

#include <string>

#include "LoginParams.h"
#include "Order.h"
#include "TraderInterface.h"

namespace ft {

class MdReceiverInterface {
 public:
  virtual ~MdReceiverInterface() {
  }

  virtual void register_cb(TraderInterface* trader) {
  }

  virtual bool login(const LoginParams& params) {
  }

  virtual bool logout() {
  }

  virtual void join() {
  }
};

}  // namespace ft

#endif  // FT_INCLUDE_MDRECEIVERINTERFACE_H_
