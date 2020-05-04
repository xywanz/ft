// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_INCLUDE_TRADINGPANEL_H_
#define FT_INCLUDE_TRADINGPANEL_H_

#include <map>
#include <string>
#include <vector>

#include "Base/DataStruct.h"

namespace ft {

class TradingPanel {
 public:
  void process_account(const Account* account) { account_ = *account; }

  void process_position(const Position* pos) { portfolio_.init_position(*pos); }

  void update_account(int64_t balance_changed) {
    account_.balance += balance_changed;
  }

  void update_float_pnl(const std::string& ticker, double price) {
    portfolio_.update_float_pnl(ticker, price);
  }

  void update_pos_traded(const std::string& ticker, Direction direction,
                         Offset offset, int64_t traded, double traded_price) {
    portfolio_.update_traded(ticker, direction, offset, traded, traded_price);
  }

  void update_pos_pending(const std::string& ticker, Direction direction,
                          Offset offset, int64_t changed) {
    portfolio_.update_pending(ticker, direction, offset, changed);
  }

  void process_new_order(const Order* order) {
    orders_.emplace(order->order_id, *order);
  }

  void update_order(const Order* rtn_order);

  void process_new_trade(const Trade* trade) {
    trade_record_.emplace(trade->ticker, *trade);
  }

  const Order* get_order_by_id(const std::string& order_id) {
    auto iter = orders_.find(order_id);
    if (iter == orders_.end()) return nullptr;
    return &iter->second;
  }

  const Order* get_order_id_list(std::vector<const std::string*>* out,
                                 const std::string& ticker = "") const {
    if (ticker.empty()) {
      for (const auto& [order_id, order] : orders_)
        out->emplace_back(&order_id);
    } else {
      for (const auto& [order_id, order] : orders_) {
        if (order.ticker == ticker) out->emplace_back(&order_id);
      }
    }
  }

  void get_order_list(std::vector<const Order*>* out,
                      const std::string& ticker = "") const {
    if (ticker.empty()) {
      for (const auto& [order_id, order] : orders_) out->emplace_back(&order);
    } else {
      for (const auto& [order_id, order] : orders_) {
        if (order.ticker == ticker) out->emplace_back(&order);
      }
    }
  }

  const Account* get_account() const { return &account_; }

  double get_realized_pnl() const { return portfolio_.get_realized_pnl(); }

  double get_float_pnl() const { return portfolio_.get_float_pnl(); }

  void get_pos_ticker_list(std::vector<const std::string*>* out) const {
    portfolio_.get_pos_ticker_list_unsafe(out);
  }

  const Position* get_position(const std::string& ticker) const {
    return portfolio_.get_position_unsafe(ticker);
  }

 private:
  Account account_;
  Portfolio portfolio_;
  std::map<std::string, Order> orders_;
  std::map<std::string, Trade> trade_record_;
};

}  // namespace ft

#endif  // FT_INCLUDE_TRADINGPANEL_H_
