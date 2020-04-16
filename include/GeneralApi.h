// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_INCLUDE_GENERALAPI_H_
#define FT_INCLUDE_GENERALAPI_H_

#include <string>

#include "Engine.h"

namespace ft {

class GeneralApi {
 public:
  explicit GeneralApi(Engine* engine)
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

  virtual AsyncStatus query_contract(const std::string& symbol,
                                     const std::string& exchange) {
    return AsyncStatus();
  }

  virtual AsyncStatus query_position(const std::string& symbol,
                                     const std::string& exchange) {
    return AsyncStatus();
  }

  virtual AsyncStatus query_account() {
    return AsyncStatus();
  }

  virtual void join() {}

    /*
   * 接收查询到的汇总仓位数据
   * 当gateway触发了一次仓位查询时，需要把仓位缓存并根据{ticker-direction}
   * 进行汇总，每个{ticker-direction}对应一个Position对象，本次查询完成后，
   * 对每个汇总的Position对象回调Trader::on_position
   */
  virtual void on_position(const Position* position) {
    if (engine_)
      engine_->on_position(position);
  }

  /*
   * 接收查询到的账户信息
   */
  virtual void on_account(const Account* account) {
    if (engine_)
      engine_->on_account(account);
  }

  virtual void on_contract(const Contract* contract) {
    if (engine_)
      engine_->on_contract(contract);
  }

  /*
   * 接受订单信息
   * 当订单状态发生改变时触发
   */
  virtual void on_order(const Order* order) {
    if (engine_)
      engine_->on_order(order);
  }

  /*
   * 接受成交信息
   * 每笔成交都会回调
   */
  virtual void on_trade(const Trade* trade) {
    if (engine_)
      engine_->on_trade(trade);
  }

  /*
   * 接受行情数据
   * 每一个tick都会回调
   */
  virtual void on_tick(const MarketData* data) {
    if (engine_)
      engine_->on_tick(data);
  }

 private:
  Engine* engine_;
};

}  // namespace ft

#endif  // FT_INCLUDE_GENERALAPI_H_
