// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_INCLUDE_CORE_ERRORCODE_H_
#define FT_INCLUDE_CORE_ERRORCODE_H_

#include <cstdint>

namespace ft {

/*
 * 错误码分为几个阶段的错误码：
 * 1. 发送前
 * 2. 发送错误
 * 3. 发送后
 */
enum ErrorCode : int {
  NO_ERROR = 0,

  // 小于ERR_SEND_FAILED的错误归属于RM
  ERR_SELF_TRADE,
  ERR_POSITION_NOT_ENOUGH,
  ERR_THROTTLE_RATE_LIMIT,

  ERR_SEND_FAILED,

  ERR_REJECTED,
  ERR_COUNT
};

inline const char* error_code_str(int error_code) {
  static const char* err_str[ERR_COUNT] = {
      "NO_ERROR",
      "ERR_SELF_TRADE",
      "ERR_POSITION_NOT_ENOUGH",
      "ERR_THROTTLE_RATE_LIMIT",
      "ERR_SEND_FAILED",
      "ERR_REJECTED",
  };

  if (error_code < 0 || error_code >= ERR_COUNT) return "UNKNOWN_ERROR_CODE";

  return err_str[error_code];
}

}  // namespace ft

#endif  // FT_INCLUDE_CORE_ERRORCODE_H_
