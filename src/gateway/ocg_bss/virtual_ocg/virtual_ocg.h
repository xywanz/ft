// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef BSS_VIRTUAL_OCG_VIRTUAL_OCG_H_
#define BSS_VIRTUAL_OCG_VIRTUAL_OCG_H_

#include <sys/socket.h>

#include <string>

// #include <AllUtils.hpp>

#include "broker/misc.h"
#include "broker/session_config.h"
#include "virtual_ocg/ocg_encoder.h"
#include "virtual_ocg/ocg_parser.h"
#include "virtual_ocg/ocg_state.h"

using namespace ft::bss;

class VirtualOcg : public OcgHandler {
 public:
  void Init();

  void listen(int primary_port, int secondary_port);

  void accept();

  void run();

  void on_logon_msg(MessageHeader* header, LogonMessage* msg) override;

  void on_logout_msg(MessageHeader* header, LogoutMessage* msg) override;

  void on_heartbeat_msg(MessageHeader* header, HeartbeatMessage* msg) override;

  void on_test_request(MessageHeader* header, TestRequest* msg) override;

  void on_resend_request(MessageHeader* header, ResendRequest* msg) override;

  void on_reject_msg(MessageHeader* header, RejectMessage* msg) override;

  void on_sequence_reset_msg(MessageHeader* header, SequenceResetMessage* msg) override;

  void on_new_order_request(MessageHeader* header, NewOrderRequest* msg) override;

  void on_amend_request(MessageHeader* header, AmendRequest* msg) override;

  void on_cancel_request(MessageHeader* header, CancelRequest* msg) override;

  void on_mass_cancel_request(MessageHeader* header, MassCancelRequest* msg) override;

  void on_obo_cancel_request(MessageHeader* header, OboCancelRequest* msg) override;

  void on_obo_mass_cancel_request(MessageHeader* header, OboMassCancelRequest* msg) override;

  void on_quote_request(MessageHeader* header, QuoteRequest* msg) override;

  void on_quote_cancel_request(MessageHeader* header, QuoteCancelRequest* msg) override;

  void on_quote_cancel_request(MessageHeader* header, TradeCaptureReport* msg) override;

  void on_party_entitlement_request(MessageHeader* header, PartyEntitlementRequest* msg) override;

  void on_invalid_msg() override;

 private:
  void process_msg(MessageHeader* header, LogonMessage* msg);

  void process_msg(MessageHeader* header, LogoutMessage* msg);

  void process_msg(MessageHeader* header, HeartbeatMessage* msg);

  void process_msg(MessageHeader* header, TestRequest* msg);

  void process_msg(MessageHeader* header, ResendRequest* msg);

  void process_msg(MessageHeader* header, RejectMessage* msg);

  void process_msg(MessageHeader* header, SequenceResetMessage* msg);

  void process_msg(MessageHeader* header, NewOrderRequest* msg);

  void process_msg(MessageHeader* header, AmendRequest* msg);

  void process_msg(MessageHeader* header, CancelRequest* msg);

  void process_msg(MessageHeader* header, MassCancelRequest* msg);

  void process_msg(MessageHeader* header, OboCancelRequest* msg);

  void process_msg(MessageHeader* header, OboMassCancelRequest* msg);

  void process_msg(MessageHeader* header, QuoteRequest* msg);

  void process_msg(MessageHeader* header, QuoteCancelRequest* msg);

  void process_msg(MessageHeader* header, TradeCaptureReport* msg);

  void process_msg(MessageHeader* header, PartyEntitlementRequest* msg);

  bool is_msg_queue_empty() const { return state_.is_msg_queue_empty(); }

  void consume_cached_msgs();

  template <class Message>
  void send_raw_msg(const Message& msg) {
    MsgBuffer msg_buffer;

    encoder_.set_next_seq_number(state_.next_send_msg_seq);
    encoder_.encode_msg(msg, &msg_buffer);

    ::send(sockfd_, msg_buffer.data, msg_buffer.size, 0);

    state_.record_sent_msg(msg);
    state_.last_send_time_ms = now_ms();
    printf("send_raw_msg. seq:%u\n", state_.next_send_msg_seq);
    ++state_.next_send_msg_seq;
  }

  void send_logon_msg();

  void send_logout_msg(BssSessionStatus status = SESSION_STATUS_OTHER,
                       const std::string& text = "");

  void send_heartbeat(TestRequestId test_req_id = 0) {
    HeartbeatMessage hearbeat{test_req_id};
    send_raw_msg(hearbeat);
    printf("send_heartbeat\n");
  }

  void send_test_request() {
    TestRequest test_req{state_.next_test_request_id};
    send_raw_msg(test_req);

    ++state_.next_test_request_id;
    state_.last_test_request_time_ms = now_ms();
    state_.sent_test_request = true;
    printf("send_test_request. test_req_id:%u\n", test_req.test_request_id);
  }

  void send_gap_fill(uint32_t new_seq_num) {
    printf("send_gap_fill: seq:%u new_seq:%u\n", state_.next_send_msg_seq, new_seq_num);

    SequenceResetMessage seq_reset_msg{'Y', new_seq_num};
    send_raw_msg(seq_reset_msg);
    state_.next_send_msg_seq = new_seq_num;
  }

  void send_reject_msg(const MessageHeader& err_msg_header, RejectCode reject_code,
                       const std::string& err_field_name = "") {
    RejectMessage reject_msg{};
    reject_msg.message_reject_code = reject_code;
    reject_msg.reference_sequence_number = err_msg_header.message_type;
    strncpy(reject_msg.reference_field_name, err_field_name.c_str(), sizeof(ReferenceFieldName));

    send_raw_msg(reject_msg);
  }

  void send_resend_request(uint32_t start_seq, uint32_t end_seq) {
    ResendRequest resend_req{start_seq, end_seq};
    send_raw_msg(resend_req);

    state_.set_resend_range(start_seq, end_seq);
    printf("send_resend_request: [%u, %u]\n", start_seq, end_seq);
  }

  void resend(uint32_t start_seq_num, uint32_t end_seq_num);

  template <class Message>
  bool verify_msg(const MessageHeader& header, const Message& msg, bool check_too_high = true,
                  bool check_too_low = true);

  bool validate_logon_state(const MessageHeader& header) const {
    return (header.message_type == LOGON && !state_.enabled && !state_.received_logon &&
            !state_.sent_logon) ||
           (header.message_type != LOGON && state_.enabled && state_.received_logon &&
            state_.sent_logon);
  }

  bool is_correct_comp_id(const CompId& comp_id) const { return comp_id_ == comp_id; }

  template <class Message>
  bool handle_msg_seq_too_high(const MessageHeader& header, const Message& msg);

  bool handle_msg_seq_too_low(const MessageHeader& header) {
    // 收到MsgSeqNum比ExpectedSeqNum小的登录回应应该告知对方应收的SeqNum并断开连接
    if (header.message_type == LOGON || header.poss_dup_flag == 0) {
      printf("failed. seq num is too low, received:%u expected:%u\n", header.sequence_number,
             state_.next_recv_msg_seq);
      std::string logout_reason = kReasonLocalSndSeqLessThanOcgExpected;
      logout_reason += std::to_string(state_.next_recv_msg_seq);
      send_logout_msg(SESSION_STATUS_OTHER, logout_reason);
      return false;
    }

    handle_poss_dup(header);
    return true;
  }

  void handle_poss_dup(const MessageHeader& header) {
    if (header.message_type != SEQUENCE_RESET) {
      // todo: do sth?
    }
  }

  void disconnect() {
    state_.reset_state();
    should_disconnect_ = true;
  }

  void periodically_check();

  uint32_t next_order_id() { return next_order_id_++; }
  uint32_t next_execution_id() { return next_execution_id_++; }

  inline void get_transaction_time(TransactionTime trans_time) {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    int hour = static_cast<int>((ts.tv_sec % 86400 / 3600));
    int min = static_cast<int>((ts.tv_sec / 60) % 60);
    int sec = static_cast<int>(ts.tv_sec % 60);
    snprintf(trans_time, sizeof(TransactionTime), "%s-%02d:%02d:%02d.%03d", date_, hour, min, sec,
             static_cast<int>(ts.tv_nsec / 1000000));
  }

 private:
  struct ResendVisitor {
    void Init(VirtualOcg* self) { self_ = self; }

    template <class Message>
    void operator()(Message& msg) {
      self_->send_raw_msg(msg);
    }

   private:
    VirtualOcg* self_;
  };

  struct ConsumerVisitor {
    void Init(VirtualOcg* self) { self_ = self; }

    void set_header(MessageHeader* header) { header_ = header; }

    template <class Message>
    void operator()(Message& msg) {
      self_->process_msg(header_, &msg);
    }

   private:
    VirtualOcg* self_;
    MessageHeader* header_;
  };

 private:
  int servfd_primary_{-1};
  int servfd_secondary_{-1};
  int sockfd_;
  int timerfd_;

  OcgParser parser_;
  OcgEncoder encoder_;
  OcgState state_;
  bool should_disconnect_ = false;
  std::string comp_id_ = "CO99999902";
  char date_[10];

  ResendVisitor resend_visitor_;
  ConsumerVisitor consumer_visitor_;

  uint32_t next_order_id_ = 1;
  uint32_t next_execution_id_ = 1;
};

template <class Message>
bool VirtualOcg::verify_msg(const MessageHeader& header, const Message& msg, bool check_too_high,
                            bool check_too_low) {
  if (!validate_logon_state(header)) {
    printf("failed. validate_logon_state\n");
    goto error;
  }

  if (!is_correct_comp_id(header.comp_id)) {
    // don't send other messages if not logon. just disconnect
    if (!state_.received_logon) {
      printf("failed. !state_.received_logon\n");
      goto error;
    }

    send_reject_msg(header, MSG_REJECT_CODE_COMP_ID_PROBLEM, "Comp ID");
    send_logout_msg();
    printf("failed. !is_correct_comp_id\n");
    goto error;
  }

  if (check_too_high && header.sequence_number > state_.next_recv_msg_seq) {
    if (!handle_msg_seq_too_high(header, msg)) {
      printf("failed. !handle_msg_seq_too_high\n");
      goto error;
    }
    return false;
  }

  if (check_too_low && header.sequence_number < state_.next_recv_msg_seq) {
    if (!handle_msg_seq_too_low(header)) {
      printf("failed. !handle_msg_seq_too_low\n");
      goto error;
    }
    return false;
  }

  if (state_.is_recovering()) {
    if (header.sequence_number >= state_.resend_range.second) {
      // ResendRequest has been satisfied
      printf("ResendRequest[%u, %u] has been satisfied\n", state_.resend_range.first,
             state_.resend_range.second);
      state_.set_resend_range(0, 0);
    }
  }

  state_.last_received_time_ms = now_ms();
  return true;

error:
  disconnect();
  return false;
}

template <class Message>
bool VirtualOcg::handle_msg_seq_too_high(const MessageHeader& header, const Message& msg) {
  // todo: To be or not to be, that is the question
  printf("handle_msg_seq_too_high: seq:%u type:%u\n", header.sequence_number, header.message_type);

  // 提前到的消息先缓存下来
  state_.cache_early_arriving_msg(header.sequence_number, header, msg);

  // 如果正在重传状态
  if (state_.is_recovering()) {
    // 如果消息在重传范围外，打印警告信息，因为不允许在重传状态下再次发起重传
    if (header.sequence_number < state_.resend_range.first &&
        header.sequence_number > state_.resend_range.second)
      printf("warn. handle_msg_seq_too_high. msg outside the resend range\n");

    return true;
  }

  // 发起重传请求
  send_resend_request(state_.next_recv_msg_seq, header.sequence_number - 1);
  return true;
}

#endif  // BSS_VIRTUAL_OCG_VIRTUAL_OCG_H_
