// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_INCLUDE_CORE_ACCOUNT_H_
#define FT_INCLUDE_CORE_ACCOUNT_H_

#include <cstdint>

namespace ft {

struct Account {
  uint64_t account_id;  // 资金账户号
  double balance;       // 余额
  double frozen;        // 冻结金额
  double margin;
};

}  // namespace ft

#endif  // FT_INCLUDE_CORE_ACCOUNT_H_
