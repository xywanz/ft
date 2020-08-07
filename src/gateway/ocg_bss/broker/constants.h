// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef BSS_BROKER_CONSTANTS_H_
#define BSS_BROKER_CONSTANTS_H_

namespace ft::bss {

// Lookup service请求失败之后的重试间隔
#define LOOKUP_SERVICE_RETRY_INTERVAL_SEC 5

// 超时没有收到登录回应导致断线，应该导致60秒后再尝试重连
#define RECONNECT_INTERVAL_SEC_IF_NOT_RSP 60

// 一般情况下的断线重连间隔
#define RECONNECT_INTERVAL_SEC 10

// 心跳间隔
#define HEARTBEAT_INTERVAL_MS (20 * 1000)

// Test Request间隔
#define TEST_REQUEST_INTERVAL_MS (3 * HEARTBEAT_INTERVAL_MS)

// Test Request超时时间
#define TEST_REQUEST_TIMEOUT_MS (3 * HEARTBEAT_INTERVAL_MS)

// 登录超时时间，若发出登录请求后，在超时时间内未收到登录回应则超时
#define LOGON_TIMEOUT_MS (60 * 1000)

// 登出超时时间，若主动发出登出请求后，在超时时间内未收到登出回应则超时
#define LOGOUT_TIMEOUT_MS (60 * 1000)

// 每个交易日的初始发送端序列号
#define DAILY_INITIAL_SND_MSG_SEQ 1

// 每个交易日的初始接收端序列号
#define DAILY_INITIAL_RCV_MSG_SEQ 1

// 每个交易日的初始Test Request ID
#define DAILY_INITIAL_TEST_REQUEST_ID 1

inline const char* const kReasonLocalSndSeqLessThanOcgExpected =
    "Sequence number is less than expected : Expected=";

inline const char* const kReasonExpectedLargerThanOcgSndSeq =
    "Invalid next expected msg sequence : Expected=";

}  // namespace ft::bss

#endif  // BSS_BROKER_CONSTANTS_H_
