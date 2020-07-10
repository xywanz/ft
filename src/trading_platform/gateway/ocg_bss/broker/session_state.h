// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef OCG_BSS_BROKER_SESSION_STATE_H_
#define OCG_BSS_BROKER_SESSION_STATE_H_

#include <chrono>
#include <cstdint>
#include <unordered_map>
#include <variant>

#include "broker/constants.h"
#include "protocol/protocol_encoder.h"

namespace ft::bss {

struct HistoricalSentMessage {
  MessageType type;
  std::variant<LogonMessage, LogoutMessage, HeartbeatMessage, TestRequest,
               ResendRequest, RejectMessage, SequenceResetMessage,
               NewOrderRequest, AmendRequest, CancelRequest, MassCancelRequest,
               OboCancelRequest, OboMassCancelRequest, QuoteRequest,
               QuoteCancelRequest, TradeCaptureReport, PartyEntitlementRequest>
      body;
};

struct CachedRecvMessage {
  MessageHeader header;
  std::variant<LogonMessage, LogoutMessage, HeartbeatMessage, TestRequest,
               ResendRequest, RejectMessage, SequenceResetMessage,
               TradeCaptureReport, ExecutionReport, TradeCaptureReportAck,
               QuoteStatusReport, OrderMassCancelReport, BusinessRejectMessage>
      body;
};

class SessionState {
 public:
  /*
   * 恢复到未登录的状态，而非恢复至初始状态
   */
  void reset_on_logout() {
    sent_logon = false;
    received_logon = false;
    sent_logout = false;
    received_logout = false;
    sent_reset = false;
    received_reset = false;
    sent_test_request = false;
    resend_range.first = resend_range.second = 0;
    received_messages.clear();
  }

  /*
   * 恢复至初始状态，当人工通知OCG重置状态后，BSS也应当重置至初始状态
   */
  void reset() {
    reset_on_logout();
    enabled = false;
    last_received_time_ms = last_send_time_ms = last_test_request_time_ms = 0;
    next_send_msg_seq = DAILY_INITIAL_SND_MSG_SEQ;
    next_recv_msg_seq = DAILY_INITIAL_RCV_MSG_SEQ;
    next_test_request_id = DAILY_INITIAL_TEST_REQUEST_ID;
    sent_messages.clear();
  }

  void set_resend_range(uint32_t start, uint32_t end) {
    resend_range.first = start;
    resend_range.second = end;
  }

  bool is_recovering() const {
    return resend_range.first > 0 || resend_range.second > 0;
  }

  bool need_heartbeat(uint64_t now_ms) const {
    return now_ms >= last_send_time_ms + heartbeat_interval_ms;
  }

  bool need_test_request(uint64_t now_ms) const {
    return now_ms >= last_received_time_ms + test_request_interval_ms &&
           !sent_test_request;
  }

  bool is_logon_timeout(uint64_t now_ms) const {
    return now_ms >= last_received_time_ms + logon_timeout_ms;
  }

  bool is_logout_timeout(uint64_t now_ms) const {
    return sent_logout && now_ms >= last_send_time_ms + logout_timeout_ms;
  }

  bool is_test_request_timeout(uint64_t now_ms) const {
    return sent_test_request &&
           now_ms >= last_test_request_time_ms + test_request_timeout_ms;
  }

  bool is_session_timeout(uint64_t now_ms) const {
    return now_ms > last_received_time_ms + test_request_interval_ms +
                        test_request_timeout_ms;
  }

  template <class Message>
  void record_sent_msg(const Message& msg) {
    auto iter = sent_messages.find(next_send_msg_seq);
    if (iter == sent_messages.end()) {
      HistoricalSentMessage sent_msg;
      sent_msg.type = get_msg_type<Message>();
      sent_msg.body = msg;
      sent_messages.emplace(next_send_msg_seq, sent_msg);
    }
  }

  template <class Message>
  void cache_early_arriving_msg(uint32_t seq, const MessageHeader& header,
                                const Message& msg) {
    auto iter = received_messages.find(seq);
    if (iter == received_messages.end()) {
      CachedRecvMessage cached_msg;
      cached_msg.header = header;
      cached_msg.body = msg;
      received_messages.emplace(seq, cached_msg);
    }
  }

  bool retrieve_msg_cache(uint32_t seq, CachedRecvMessage* msg) {
    auto iter = received_messages.find(seq);
    if (iter == received_messages.end()) return false;
    *msg = iter->second;
    received_messages.erase(iter);
    return true;
  }

  bool is_msg_queue_empty() const { return received_messages.empty(); }

  bool enabled = false;
  bool sent_logon = false;
  bool received_logon = false;
  bool sent_logout = false;
  bool received_logout = false;
  bool sent_reset = false;
  bool received_reset = false;
  bool sent_test_request = false;

  uint64_t last_received_time_ms;
  uint64_t last_send_time_ms;
  uint64_t last_test_request_time_ms;

  std::pair<uint32_t, uint32_t> resend_range{0, 0};

  uint64_t heartbeat_interval_ms = HEARTBEAT_INTERVAL_MS;
  uint64_t test_request_interval_ms = TEST_REQUEST_INTERVAL_MS;
  uint64_t test_request_timeout_ms = TEST_REQUEST_TIMEOUT_MS;
  uint64_t logon_timeout_ms = LOGON_TIMEOUT_MS;
  uint64_t logout_timeout_ms = LOGOUT_TIMEOUT_MS;

  uint32_t next_send_msg_seq = DAILY_INITIAL_SND_MSG_SEQ;
  uint32_t next_recv_msg_seq = DAILY_INITIAL_RCV_MSG_SEQ;

  TestRequestId next_test_request_id = DAILY_INITIAL_TEST_REQUEST_ID;

  std::unordered_map<uint32_t, HistoricalSentMessage> sent_messages;
  std::unordered_map<uint32_t, CachedRecvMessage> received_messages;
};

}  // namespace ft::bss

#endif  // OCG_BSS_BROKER_SESSION_STATE_H_
