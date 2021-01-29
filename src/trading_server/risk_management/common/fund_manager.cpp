// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "trading_server/risk_management/common/fund_manager.h"

#include <spdlog/spdlog.h>

#include "trading_server/datastruct/contract_table.h"

namespace ft {

bool FundManager::Init(RiskRuleParams* params) {
  account_ = params->account;
  return true;
}

int FundManager::CheckOrderRequest(const Order* order) {
  // 暂时只针对买卖进行管理，申赎等操作由其他模块计算资金占用
  // 融资融券暂不支持
  // TODO(Kevin): 市价单不会进行资金的预先冻结，因为不知道价格，这算是个bug
  if (order->req.direction != Direction::BUY && order->req.direction != Direction::SELL)
    return NO_ERROR;

  auto* req = &order->req;
  if (IsOffsetClose(req->offset)) return NO_ERROR;

  auto contract = ContractTable::get_by_index(req->contract->ticker_id);
  assert(contract);
  assert(contract->size > 0);

  double estimated = 0;
  if (req->direction == Direction::BUY) {
    estimated = req->price * req->volume * contract->size * contract->long_margin_rate;
  } else if (req->direction == Direction::SELL) {
    estimated = req->price * req->volume * contract->size * contract->short_margin_rate;
  }

  if (account_->cash * 1.1 < estimated) return ERR_FUND_NOT_ENOUGH;

  return NO_ERROR;
}

void FundManager::OnOrderSent(const Order* order) {
  auto contract = order->req.contract;

  if (IsOffsetOpen(order->req.offset)) {
    auto margin_rate = order->req.direction == Direction::BUY ? contract->long_margin_rate
                                                              : contract->short_margin_rate;
    double changed = contract->size * order->req.volume * order->req.price * margin_rate;
    account_->cash -= changed;
    account_->frozen += changed;
    spdlog::debug("Account: balance:{:.3f} frozen:{:.3f} margin:{:.3f}", account_->total_asset,
                  account_->frozen, account_->margin);
  }
}

void FundManager::OnOrderTraded(const Order* order, const Trade* trade) {
  if (trade->trade_type == TradeType::SECONDARY_MARKET) {
    auto contract = order->req.contract;

    if (IsOffsetOpen(order->req.offset)) {
      auto margin_rate = order->req.direction == Direction::BUY ? contract->long_margin_rate
                                                                : contract->short_margin_rate;
      auto frozen_released = contract->size * trade->volume * order->req.price * margin_rate;
      auto margin = contract->size * trade->volume * trade->price * margin_rate;
      account_->frozen -= frozen_released;
      account_->margin += margin;
      // 如果冻结的时候冻结多了需要释放，冻结少了则需要回补
      account_->cash += frozen_released - margin;
    } else if (IsOffsetClose(order->req.offset)) {
      auto margin_rate = order->req.direction == Direction::BUY ? contract->long_margin_rate
                                                                : contract->short_margin_rate;
      auto margin = contract->size * trade->volume * trade->price * margin_rate;
      account_->margin -= margin;
      account_->cash += margin;
      if (account_->margin < 0) account_->margin = 0;
      spdlog::debug("Account: balance:{:.3f} frozen:{:.3f} margin:{:.3f}", account_->total_asset,
                    account_->frozen, account_->margin);
    }
  } else if (trade->trade_type == TradeType::CASH_SUBSTITUTION) {
    if (order->req.direction == Direction::PURCHASE) {
      account_->margin -= trade->amount;
      account_->cash += trade->amount;
    } else if (order->req.direction == Direction::REDEEM) {
      account_->margin += trade->amount;
      account_->cash -= trade->amount;
    }
  }
}

void FundManager::OnOrderCanceled(const Order* order, int canceled) {
  if (IsOffsetOpen(order->req.offset)) {
    auto contract = order->req.contract;
    auto margin_rate = order->req.direction == Direction::BUY ? contract->long_margin_rate
                                                              : contract->short_margin_rate;
    double changed = contract->size * canceled * order->req.price * margin_rate;
    account_->frozen -= changed;
    account_->cash += changed;

    spdlog::debug("Account: balance:{:.3f} frozen:{:.3f} margin:{:.3f}", account_->total_asset,
                  account_->frozen, account_->margin);
  }
}

void FundManager::OnOrderRejected(const Order* order, int error_code) {
  if (error_code <= ERR_SEND_FAILED) return;

  OnOrderCanceled(order, order->req.volume);
}

}  // namespace ft
