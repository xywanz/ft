// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "RiskManagement/AvailableFundCheck.h"

#include "Core/Constants.h"
#include "Core/ContractTable.h"

namespace ft {

AvailableFundCheck::AvailableFundCheck(const Account* account)
    : account_(account) {}

int AvailableFundCheck::check_order_req(const OrderReq* order) {
  if (is_offset_close(order->offset)) return NO_ERROR;

  auto contract = ContractTable::get_by_index(order->ticker_index);
  assert(contract);
  assert(contract->size > 0);

  double avl = account_->balance - account_->frozen - account_->margin;
  double estimated;
  if (order->direction == Direction::BUY) {
    estimated = order->price * order->volume * contract->size *
                contract->long_margin_rate;
  } else if (order->direction == Direction::SELL) {
    estimated = order->price * order->volume * contract->size *
                contract->short_margin_rate;
  } else {
    assert(false);
  }

  if (avl < estimated) return ERR_FUND_NOT_ENOUGH;

  return NO_ERROR;
}

}  // namespace ft
