// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_INCLUDE_GATEWAYINTERFACE_H_
#define FT_INCLUDE_GATEWAYINTERFACE_H_

#include <string>

#include "LoginParams.h"
#include "Order.h"
#include "TraderInterface.h"

namespace ft {

class GatewayInterface {
 public:
  virtual ~GatewayInterface() {
  }

  virtual void register_cb(TraderInterface* cb) {
  }

  virtual bool login(const LoginParams& params) {
  }

  virtual bool logout() {
  }

  virtual std::string send_order(const Order* order) {
  }

  virtual bool cancel_order(const std::string& order_id) {
  }

  virtual AsyncStatus query_contract(const std::string& symbol,
                                    const std::string& exchange) {
  }

  virtual AsyncStatus query_position(const std::string& symbol,
                                    const std::string& exchange) {
  }

  virtual AsyncStatus query_account() {
  }

  virtual void join() {
  }
};

}  // namespace ft

#endif  // FT_INCLUDE_GATEWAYINTERFACE_H_
