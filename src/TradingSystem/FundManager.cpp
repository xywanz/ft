// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "TradingSystem/FundManager.h"

#include <spdlog/spdlog.h>

#include "Core/ContractTable.h"

namespace ft {

void FundManager::init(Account* account) { account_ = account; }
void FundManager::on_new_order(const Order& order) {
  if (!account_) return;

  auto contract = order.contract;

  if (is_offset_open(order.offset)) {
    auto margin_rate = order.direction == Direction::BUY
                           ? contract->long_margin_rate
                           : contract->short_margin_rate;
    account_->frozen +=
        contract->size * order.volume * order.price * margin_rate;
    spdlog::debug("Account: balance:{} frozen:{} margin:{}", account_->balance,
                  account_->frozen, account_->margin);
  }
}

void FundManager::on_order_abort(const Order& order, int incompleted_volume) {
  if (!account_) return;

  if (is_offset_open(order.offset)) {
    auto contract = order.contract;
    auto margin_rate = order.direction == Direction::BUY
                           ? contract->long_margin_rate
                           : contract->short_margin_rate;
    account_->frozen -=
        contract->size * incompleted_volume * order.price * margin_rate;
    spdlog::debug("Account: balance:{} frozen:{} margin:{}", account_->balance,
                  account_->frozen, account_->margin);
  }
}

void FundManager::on_order_traded(const Order& order, int traded,
                            double traded_price) {
  if (!account_) return;

  auto contract = order.contract;
  if (is_offset_open(order.offset)) {
    auto margin_rate = order.direction == Direction::BUY
                           ? order.contract->long_margin_rate
                           : order.contract->short_margin_rate;
    auto frozen_released = contract->size * traded * order.price * margin_rate;
    auto margin = order.contract->size * traded * traded_price * margin_rate;
    account_->frozen -= frozen_released;
    account_->margin += margin;
  } else if (is_offset_close(order.offset)) {
    auto margin_rate = order.direction == Direction::BUY
                           ? order.contract->long_margin_rate
                           : order.contract->short_margin_rate;
    auto margin = order.contract->size * traded * traded_price * margin_rate;
    account_->margin -= margin;
    if (account_->margin < 0) account_->margin = 0;
    spdlog::debug("Account: balance:{} frozen:{} margin:{}", account_->balance,
                  account_->frozen, account_->margin);
  }
}

}  // namespace ft
