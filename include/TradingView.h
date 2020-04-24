// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_INCLUDE_TRADINGVIEW_H_
#define FT_INCLUDE_TRADINGVIEW_H_

#include <map>
#include <string>
#include <vector>

#include "Account.h"
#include "Common.h"
#include "Order.h"
#include "Position.h"
#include "Trade.h"

namespace ft {

class TradingView {
 public:
  void on_query_account(const Account* account) {
    account_ = *account;
  }

  void on_query_position(const Position* pos) {
    pos_mgr_.init_position(*pos);
  }

  void update_account(int64_t balance_changed) {
    account_.balance += balance_changed;
  }

  void update_pos_pnl(const std::string& ticker, double price) {
    pos_mgr_.update_pnl(ticker, price);
  }

  void update_pos_traded(const std::string& ticker, Direction direction,
                         Offset offset, int64_t traded, double traded_price) {
    pos_mgr_.update_traded(ticker, direction, offset, traded, traded_price);
  }

  void update_pos_pending(const std::string& ticker, Direction direction,
                          Offset offset, int64_t changed) {
    pos_mgr_.update_pending(ticker, direction, offset, changed);
  }

  void new_order(const Order* order) {
    orders_.emplace(order->order_id, *order);
  }

  void update_order(const Order* rtn_order);

  void new_trade(const Trade* trade) {
    trade_record_.emplace(trade->ticker, *trade);
  }

  const Order* get_order_by_id(const std::string& order_id) {
    auto iter = orders_.find(order_id);
    if (iter == orders_.end())
      return nullptr;
    return &iter->second;
  }

  const Order* get_order_id_list(std::vector<const std::string*>* out,
                                 const std::string& ticker = "") const {
    if (ticker.empty()) {
      for (const auto& [order_id, order] : orders_)
        out->emplace_back(&order_id);
    } else {
      for (const auto& [order_id, order] : orders_) {
        if (order.ticker == ticker)
          out->emplace_back(&order_id);
      }
    }
  }

  void get_order_list(std::vector<const Order*>* out,
                      const std::string& ticker = "") const {
    if (ticker.empty()) {
      for (const auto& [order_id, order] : orders_)
        out->emplace_back(&order);
    } else {
      for (const auto& [order_id, order] : orders_) {
        if (order.ticker == ticker)
          out->emplace_back(&order);
      }
    }
  }

  const Account* get_account() const {
    return &account_;
  }

  void get_pos_ticker_list(std::vector<const std::string*>* out) const {
    pos_mgr_.get_pos_ticker_list_unsafe(out);
  }

  const Position* get_position(const std::string& ticker) const {
    return pos_mgr_.get_position_unsafe(ticker);
  }

 private:
  void handle_canceled(const Order* rtn_order);
  void handle_submitted(const Order* rtn_order);
  void handle_part_traded(const Order* rtn_order);
  void handle_all_traded(const Order* rtn_order);
  void handle_cancel_rejected(const Order* rtn_order);

 private:
  Account account_;
  PositionManager pos_mgr_;
  std::map<std::string, Order> orders_;
  std::map<std::string, Trade> trade_record_;
};

}  // namespace ft

#endif  // FT_INCLUDE_TRADINGVIEW_H_
