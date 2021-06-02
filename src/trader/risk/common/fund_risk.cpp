// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "trader/risk/common/fund_risk.h"

#include "ft/base/contract_table.h"
#include "ft/base/log.h"

namespace ft {

bool FundRisk::Init(RiskRuleParams* params) {
  account_ = params->account;
  LOG_INFO("fund risk inited");
  return true;
}

ErrorCode FundRisk::CheckOrderRequest(const Order& order) {
  // 暂时只针对买卖进行管理，申赎等操作由其他模块计算资金占用
  // 融资融券暂不支持
  // TODO(Kevin): 市价单不会进行资金的预先冻结，因为不知道价格，这算是个bug
  if (order.req.direction != Direction::kBuy && order.req.direction != Direction::kSell)
    return ErrorCode::kNoError;

  auto& req = order.req;
  if (IsOffsetClose(req.offset)) return ErrorCode::kNoError;

  auto contract = ContractTable::get_by_index(req.contract->ticker_id);
  assert(contract);
  assert(contract->size > 0);

  double estimated = 0;
  if (req.direction == Direction::kBuy) {
    estimated = req.price * req.volume * contract->size * contract->long_margin_rate;
  } else if (req.direction == Direction::kSell) {
    estimated = req.price * req.volume * contract->size * contract->short_margin_rate;
  }

  if (account_->cash * 1.1 < estimated) return ErrorCode::kFundNotEnough;

  return ErrorCode::kNoError;
}

void FundRisk::OnOrderSent(const Order& order) {
  auto contract = order.req.contract;

  if (IsOffsetOpen(order.req.offset)) {
    auto margin_rate = order.req.direction == Direction::kBuy ? contract->long_margin_rate
                                                              : contract->short_margin_rate;
    double changed = contract->size * order.req.volume * order.req.price * margin_rate;
    account_->cash -= changed;
    account_->frozen += changed;
    LOG_DEBUG("Account: balance:{:.3f} frozen:{:.3f} margin:{:.3f}",
              static_cast<double>(account_->total_asset), static_cast<double>(account_->frozen),
              static_cast<double>(account_->margin));
  }
}

void FundRisk::OnOrderTraded(const Order& order, const OrderTradedRsp& trade) {
  auto contract = order.req.contract;

  if (IsOffsetOpen(order.req.offset)) {
    auto margin_rate = order.req.direction == Direction::kBuy ? contract->long_margin_rate
                                                              : contract->short_margin_rate;
    auto frozen_released = contract->size * trade.volume * order.req.price * margin_rate;
    auto margin = contract->size * trade.volume * trade.price * margin_rate;
    account_->frozen -= frozen_released;
    account_->margin += margin;
    // 如果冻结的时候冻结多了需要释放，冻结少了则需要回补
    account_->cash += frozen_released - margin;
  } else if (IsOffsetClose(order.req.offset)) {
    auto margin_rate = order.req.direction == Direction::kBuy ? contract->long_margin_rate
                                                              : contract->short_margin_rate;
    auto margin = contract->size * trade.volume * trade.price * margin_rate;
    account_->margin -= margin;
    account_->cash += margin;
    if (account_->margin < 0) account_->margin = 0;
    LOG_DEBUG("Account: balance:{:.3f} frozen:{:.3f} margin:{:.3f}",
              static_cast<double>(account_->total_asset), static_cast<double>(account_->frozen),
              static_cast<double>(account_->margin));
  }
}

void FundRisk::OnOrderCanceled(const Order& order, int canceled) {
  if (IsOffsetOpen(order.req.offset)) {
    auto contract = order.req.contract;
    auto margin_rate = order.req.direction == Direction::kBuy ? contract->long_margin_rate
                                                              : contract->short_margin_rate;
    double changed = contract->size * canceled * order.req.price * margin_rate;
    account_->frozen -= changed;
    account_->cash += changed;

    LOG_DEBUG("Account: balance:{:.3f} frozen:{:.3f} margin:{:.3f}",
              static_cast<double>(account_->total_asset), static_cast<double>(account_->frozen),
              static_cast<double>(account_->margin));
  }
}

void FundRisk::OnOrderRejected(const Order& order, ErrorCode error_code) {
  OnOrderCanceled(order, order.req.volume);
}

REGISTER_RISK_RULE("ft.risk.fund", FundRisk);

}  // namespace ft
