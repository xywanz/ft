// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_SRC_TRADING_PLATFORM_COMMON_TYPES_H_
#define FT_SRC_TRADING_PLATFORM_COMMON_TYPES_H_

#include <unordered_map>

#include "cep/data/order.h"

namespace ft {

using OrderMap = std::unordered_map<uint64_t, Order>;

}

#endif  // FT_SRC_TRADING_PLATFORM_COMMON_TYPES_H_
