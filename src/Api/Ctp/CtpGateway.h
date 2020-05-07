// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_SRC_API_CTP_CTPGATEWAY_H_
#define FT_SRC_API_CTP_CTPGATEWAY_H_

#include <memory>
#include <string>
#include <vector>

#include "Api/Ctp/CtpMdApi.h"
#include "Api/Ctp/CtpTradeApi.h"
#include "Gateway.h"

namespace ft {

class CtpGateway : public Gateway {
 public:
  explicit CtpGateway(EventEngine* engine) : Gateway(engine) {
    trade_api_.reset(new CtpTradeApi(this));
    md_api_.reset(new CtpMdApi(this));
  }

  bool login(const LoginParams& params) override {
    if (!params.md_server_addr().empty()) {
      if (!md_api_->login(params)) return false;
    }

    if (!params.front_addr().empty()) {
      if (!trade_api_->login(params)) return false;
    }

    return true;
  }

  void logout() override {
    md_api_->logout();
    trade_api_->logout();
  }

  uint64_t send_order(const Order* order) override {
    return trade_api_->send_order(order);
  }

  bool cancel_order(uint64_t order_id) override {
    return trade_api_->cancel_order(order_id);
  }

  bool query_contract(const std::string& ticker) override {
    return trade_api_->query_contract(ticker);
  }

  bool query_contracts() override { return query_contract(""); }

  bool query_position(const std::string& ticker) override {
    return trade_api_->query_position(ticker);
  }

  bool query_positions() override { return query_position(""); }

  bool query_account() override { return trade_api_->query_account(); }

  bool query_margin_rate(const std::string& ticker) override {
    return trade_api_->query_margin_rate(ticker);
  }

 private:
  std::unique_ptr<CtpTradeApi> trade_api_;
  std::unique_ptr<CtpMdApi> md_api_;
};

}  // namespace ft

#endif  // FT_SRC_API_CTP_CTPGATEWAY_H_
