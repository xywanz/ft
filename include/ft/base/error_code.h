// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_INCLUDE_FT_BASE_ERROR_CODE_H_
#define FT_INCLUDE_FT_BASE_ERROR_CODE_H_

namespace ft {

/*
 * 错误码分为几个阶段的错误码：
 * 1. 发送前
 * 2. 发送错误
 * 3. 发送后
 */
enum class ErrorCode : int {
  kNoError = 0,

  // 小于ErrorCode::kSendFailed的错误归属于RM
  kSelfTrade,
  kPositionNotEnough,
  kFundNotEnough,
  kExceedThrottleRateRisk,

  kSendFailed,

  kRejected,
};

inline const char* ErrorCodeStr(ErrorCode error_code) {
  switch (error_code) {
    case ErrorCode::kNoError: {
      return "NoError";
    }
    case ErrorCode::kSelfTrade: {
      return "SelfTrade";
    }
    case ErrorCode::kPositionNotEnough: {
      return "PositionNotEnough";
    }
    case ErrorCode::kFundNotEnough: {
      return "FundNotEnough";
    }
    case ErrorCode::kExceedThrottleRateRisk: {
      return "ExceedThrottleRateRisk";
    }
    case ErrorCode::kSendFailed: {
      return "SendFailed";
    }
    case ErrorCode::kRejected: {
      return "Rejected";
    }
    default: {
      return "UnknownError";
    }
  }
}

}  // namespace ft

#endif  // FT_INCLUDE_FT_BASE_ERROR_CODE_H_
