// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "risk_management/fund_manager.h"

#include <spdlog/spdlog.h>

#include "core/contract_table.h"

namespace ft {

FundManager::FundManager(Account* account) : account_(account) {}

int FundManager::check_order_req(const Order* order) {
  auto* req = &order->req;
  if (is_offset_close(req->offset)) return NO_ERROR;

  if (!account_) return ERR_FUND_NOT_ENOUGH;

  auto contract = ContractTable::get_by_index(req->ticker_index);
  assert(contract);
  assert(contract->size > 0);

  double avl = account_->balance - account_->frozen - account_->margin;
  double estimated;
  if (req->direction == Direction::BUY) {
    estimated =
        req->price * req->volume * contract->size * contract->long_margin_rate;
  } else if (req->direction == Direction::SELL) {
    estimated =
        req->price * req->volume * contract->size * contract->short_margin_rate;
  } else {
    assert(false);
  }

  if (avl < estimated) return ERR_FUND_NOT_ENOUGH;

  return NO_ERROR;
}

void FundManager::on_order_sent(const Order* order) {
  if (!account_) return;

  auto contract = order->contract;

  if (is_offset_open(order->req.offset)) {
    auto margin_rate = order->req.direction == Direction::BUY
                           ? contract->long_margin_rate
                           : contract->short_margin_rate;
    account_->frozen +=
        contract->size * order->req.volume * order->req.price * margin_rate;
    spdlog::debug("Account: balance:{:.3f} frozen:{:.3f} margin:{:.3f}",
                  account_->balance, account_->frozen, account_->margin);
  }
}

void FundManager::on_order_traded(const Order* order, int traded,
                                  double traded_price) {
  if (!account_) return;

  auto contract = order->contract;
  if (is_offset_open(order->req.offset)) {
    auto margin_rate = order->req.direction == Direction::BUY
                           ? contract->long_margin_rate
                           : contract->short_margin_rate;
    auto frozen_released =
        contract->size * traded * order->req.price * margin_rate;
    auto margin = order->contract->size * traded * traded_price * margin_rate;
    account_->frozen -= frozen_released;
    account_->margin += margin;
  } else if (is_offset_close(order->req.offset)) {
    auto margin_rate = order->req.direction == Direction::BUY
                           ? contract->long_margin_rate
                           : contract->short_margin_rate;
    auto margin = contract->size * traded * traded_price * margin_rate;
    account_->margin -= margin;
    if (account_->margin < 0) account_->margin = 0;
    spdlog::debug("Account: balance:{:.3f} frozen:{:.3f} margin:{:.3f}",
                  account_->balance, account_->frozen, account_->margin);
  }
}

void FundManager::on_order_canceled(const Order* order, int canceled) {
  if (!account_) return;

  if (is_offset_open(order->req.offset)) {
    auto contract = order->contract;
    auto margin_rate = order->req.direction == Direction::BUY
                           ? contract->long_margin_rate
                           : contract->short_margin_rate;
    account_->frozen -=
        contract->size * canceled * order->req.price * margin_rate;
    spdlog::debug("Account: balance:{:.3f} frozen:{:.3f} margin:{:.3f}",
                  account_->balance, account_->frozen, account_->margin);
  }
}

void FundManager::on_order_rejected(const Order* order, int error_code) {
  if (error_code <= ERR_SEND_FAILED) return;

  on_order_canceled(order, order->req.volume);
}

}  // namespace ft
