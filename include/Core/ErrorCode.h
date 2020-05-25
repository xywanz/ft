// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_INCLUDE_CORE_ERRORCODE_H_
#define FT_INCLUDE_CORE_ERRORCODE_H_

#include <cstdint>

namespace ft {

enum ErrorCode : int {
  NO_ERROR = 0,
  ERR_SEND_FAILED,
  ERR_REJECTED,
};

}  // namespace ft

#endif  // FT_INCLUDE_CORE_ERRORCODE_H_
