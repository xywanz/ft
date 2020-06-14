// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_INCLUDE_CORE_ACCOUNT_H_
#define FT_INCLUDE_CORE_ACCOUNT_H_

#include <cstdint>

namespace ft {

/*
 * total_asset = cash + margin + fronzen
 * total_asset: 总资产
 * cash: 可用资金，可用于购买证券资产的资金
 * margin: 保证金，对于股票来说就是持有的股票资产（不考虑融资融券，还不支持）
 * fronzen: 冻结资金，未成交的订单也需要预先占用资金
 */
struct Account {
  uint64_t account_id;  // 资金账户号
  double total_asset;   // 总资产
  double cash;          // 可用资金
  double margin;        // 保证金
  double frozen;        // 冻结金额
};

}  // namespace ft

#endif  // FT_INCLUDE_CORE_ACCOUNT_H_
