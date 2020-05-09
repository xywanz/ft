// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_INCLUDE_TRADINGPANEL_H_
#define FT_INCLUDE_TRADINGPANEL_H_

#include <map>
#include <string>
#include <vector>

#include "Base/DataStruct.h"
#include "ContractTable.h"
#include "PositionManager.h"

namespace ft {

class TradingPanel {
 public:
  TradingPanel();

  void process_account(const Account* account) { account_ = *account; }

  void process_position(const Position* pos) { portfolio_.set_position(pos); }

  void update_account(int64_t balance_changed) {
    account_.balance += balance_changed;
  }

  void update_float_pnl(uint64_t ticker_index, double price) {
    portfolio_.update_float_pnl(ticker_index, price);
  }

  void update_pos_traded(uint64_t ticker_index, Direction direction,
                         Offset offset, int64_t traded, double traded_price) {
    portfolio_.update_traded(ticker_index, direction, offset, traded,
                             traded_price);
  }

  void update_pos_pending(uint64_t ticker_index, Direction direction,
                          Offset offset, int64_t changed) {
    portfolio_.update_pending(ticker_index, direction, offset, changed);
  }

  void process_new_order(const Order* order) {
    orders_.emplace(order->order_id, *order);
  }

  void update_order(const Order* rtn_order);

  void process_new_trade(const Trade* trade) {
    trade_record_.emplace(trade->ticker_index, *trade);
  }

  const Order* get_order_by_id(uint64_t order_id) {
    auto iter = orders_.find(order_id);
    if (iter == orders_.end()) return nullptr;
    return &iter->second;
  }

  void get_order_id_list(std::vector<uint64_t>* out,
                         const std::string& ticker = "") const {
    if (ticker.empty()) {
      for (const auto& [order_id, order] : orders_) out->emplace_back(order_id);
    } else {
      const auto* contract = ContractTable::get_by_ticker(ticker);
      if (!contract) {
        spdlog::error("[TradingPanel::get_order_id_list] Contract not found");
        return;
      }
      for (const auto& [order_id, order] : orders_) {
        if (order.ticker_index == contract->index) out->emplace_back(order_id);
      }
    }
  }

  void get_order_list(std::vector<const Order*>* out,
                      const std::string& ticker = "") const {
    if (ticker.empty()) {
      for (const auto& [order_id, order] : orders_) out->emplace_back(&order);
    } else {
      const auto* contract = ContractTable::get_by_ticker(ticker);
      if (!contract) {
        spdlog::error("[TradingPanel::get_order_list] Contract not found");
        return;
      }
      for (const auto& [order_id, order] : orders_) {
        if (order.ticker_index == contract->index) out->emplace_back(&order);
      }
    }
  }

  const Account* get_account() const { return &account_; }

  double get_realized_pnl() const { return portfolio_.get_realized_pnl(); }

  double get_float_pnl() const { return portfolio_.get_float_pnl(); }

  void get_pos_ticker_list(std::vector<uint64_t>* out) const {}

  const Position get_position(const std::string& ticker) const {
    return portfolio_.get_position(ticker);
  }

 private:
  Account account_;
  // Portfolio portfolio_;
  PositionManager portfolio_;
  std::map<uint64_t, Order> orders_;
  std::map<uint64_t, Trade> trade_record_;
};

}  // namespace ft

#endif  // FT_INCLUDE_TRADINGPANEL_H_
