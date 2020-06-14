// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "risk_management/common/fund_manager.h"

#include <spdlog/spdlog.h>

#include "core/contract_table.h"

namespace ft {

bool FundManager::init(const Config& config, Account* account,
                       Portfolio* portfolio,
                       std::map<uint64_t, Order>* order_map,
                       const MdSnapshot* md_snapshot) {
  account_ = account;
  return true;
}

int FundManager::check_order_req(const Order* order) {
  // 暂时只针对买卖进行管理，申赎等操作由其他模块计算资金占用
  // 融资融券暂不支持
  if (order->req.direction != Direction::BUY ||
      order->req.direction != Direction::SELL)
    return NO_ERROR;

  auto* req = &order->req;
  if (is_offset_close(req->offset)) return NO_ERROR;

  auto contract = ContractTable::get_by_index(req->ticker_index);
  assert(contract);
  assert(contract->size > 0);

  double avl = account_->balance - account_->frozen - account_->margin;
  double estimated = 0;
  if (req->direction == Direction::BUY) {
    estimated =
        req->price * req->volume * contract->size * contract->long_margin_rate;
  } else if (req->direction == Direction::SELL) {
    estimated =
        req->price * req->volume * contract->size * contract->short_margin_rate;
  }

  if (avl < estimated) return ERR_FUND_NOT_ENOUGH;

  return NO_ERROR;
}

void FundManager::on_order_sent(const Order* order) {
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

void FundManager::on_order_traded(const Order* order,
                                  const OrderTradedRsp* trade) {
  if (trade->trade_type == TradeType::SECONDARY_MARKET) {
    auto contract = order->contract;

    if (is_offset_open(order->req.offset)) {
      auto margin_rate = order->req.direction == Direction::BUY
                             ? contract->long_margin_rate
                             : contract->short_margin_rate;
      auto frozen_released =
          contract->size * trade->volume * order->req.price * margin_rate;
      auto margin =
          order->contract->size * trade->volume * trade->price * margin_rate;
      account_->frozen -= frozen_released;
      account_->margin += margin;
    } else if (is_offset_close(order->req.offset)) {
      auto margin_rate = order->req.direction == Direction::BUY
                             ? contract->long_margin_rate
                             : contract->short_margin_rate;
      auto margin = contract->size * trade->volume * trade->price * margin_rate;
      account_->margin -= margin;
      if (account_->margin < 0) account_->margin = 0;
      spdlog::debug("Account: balance:{:.3f} frozen:{:.3f} margin:{:.3f}",
                    account_->balance, account_->frozen, account_->margin);
    }
  } else if (trade->trade_type == TradeType::CASH_SUBSTITUTION) {
    account_->margin -= trade->amount;
  }
}

void FundManager::on_order_canceled(const Order* order, int canceled) {
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
