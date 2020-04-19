// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_INCLUDE_GENERALAPI_H_
#define FT_INCLUDE_GENERALAPI_H_

#include <memory>
#include <string>

#include "Account.h"
#include "Contract.h"
#include "EventEngine.h"
#include "LoginParams.h"
#include "MarketData.h"
#include "Order.h"
#include "Position.h"
#include "Trade.h"
namespace ft {

class GeneralApi {
 public:
  explicit GeneralApi(EventEngine* engine)
    : engine_(engine) {
  }

  virtual ~GeneralApi() {}

  virtual bool login(const LoginParams& params) {
    return false;
  }

  virtual bool logout() {
    return false;
  }

  virtual std::string send_order(const Order* order) {
    return "";
  }

  virtual bool cancel_order(const std::string& order_id) {
    return false;
  }

  virtual bool query_contract(const std::string& symbol,
                                     const std::string& exchange) {
    return false;
  }

  virtual bool query_position(const std::string& symbol,
                                     const std::string& exchange) {
    return false;
  }

  virtual bool query_account() {
    return false;
  }

    /*
   * 接收查询到的汇总仓位数据
   * 当gateway触发了一次仓位查询时，需要把仓位缓存并根据{ticker-direction}
   * 进行汇总，每个{ticker-direction}对应一个Position对象，本次查询完成后，
   * 对每个汇总的Position对象回调Trader::on_position
   */
  void on_position(const Position* position) {
    if (engine_) {
      auto* data = new Position(*position);
      engine_->post(EV_POSITION, data);
    }
  }

  /*
   * 接收查询到的账户信息
   */
  void on_account(const Account* account) {
    if (engine_) {
      auto* data = new Account(*account);
      engine_->post(EV_ACCOUNT, data);
    }
  }

  void on_contract(const Contract* contract) {
    if (engine_) {
      auto* data = new Contract(*contract);
      engine_->post(EV_CONTRACT, data);
    }
  }

  /*
   * 接受订单信息
   * 当订单状态发生改变时触发
   */
  void on_order(const Order* order) {
    if (engine_) {
      auto* data = new Order(*order);
      engine_->post(EV_ORDER, data);
    }
  }

  /*
   * 接受成交信息
   * 每笔成交都会回调
   */
  void on_trade(const Trade* trade) {
    if (engine_) {
      auto* data = new Trade(*trade);
      engine_->post(EV_TRADE, data);
    }
  }

  /*
   * 接受行情数据
   * 每一个tick都会回调
   */
  void on_tick(const MarketData* tick) {
    if (engine_) {
      auto* data = new MarketData(*tick);
      engine_->post(EV_TICK, data);
    }
  }

 private:
  EventEngine* engine_;
};

}  // namespace ft

#endif  // FT_INCLUDE_GENERALAPI_H_
