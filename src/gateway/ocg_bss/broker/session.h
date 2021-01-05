// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef OCG_BSS_BROKER_SESSION_H_
#define OCG_BSS_BROKER_SESSION_H_

#include <memory>
#include <mutex>
#include <queue>
#include <string>

#include "broker/broker.h"
#include "broker/encrypto.h"
#include "broker/misc.h"
#include "broker/session_config.h"
#include "broker/session_state.h"
#include "broker/socket_sender.h"
#include "protocol/message_handler.h"
#include "protocol/protocol_encoder.h"

namespace ft::bss {

/*
 * Session用于保证数据的完整性及正确性，不处理业务逻辑
 *
 * Session应该具备以下几个功能：
 * 1. 符合OCG文档中所描述的错误消息处理方式，在该断开连接的情况下主动断开连接
 * 2. 任何情景下的连接断开都应自动重连，且符合OCG对重连间隔的要求
 * 3. 能处理数据丢失重传的情况，能接受来自broker发来的数据重传请求
 * 4. 能根据OCG文档的说明在登录时自动重传
 * 5. 保证数据的完整性及正确性，将正确的数据传递给Broker
 *
 * 更新说明：
 * 2020/6/30:
 * 第2点已移至ConnectionManager中实现，Session断线时告知ConnectionManager
 * 断线原因，ConnectionManager根据断线原因选择重连间隔，并在相应的时间通知
 * Session让其重新发起登录请求
 */
class Session : public MessageHandler {
 public:
  explicit Session(BssBroker* broker);

  bool Init(const SessionConfig& conf);

  void set_password(const std::string& passwd, const std::string& new_passwd);

  void set_socket_sender(SocketSender* sender) { socket_sender_ = sender; }

  void reset();

  /*
   * Connection在连接建立好后会自动调用登录函数
   */
  void send_logon_msg();

  /*
   * 供业务层调用，用于发送订单相关的请求
   *
   * 如果超过了订单流速限制直接返回错误
   */
  template <class Message>
  bool send_business_msg(const Message& order_req) {
    std::unique_lock<std::mutex> lock(mutex_);
    if (is_send_vol_exceed_threshold()) {
      return false;
    }
    send_raw_msg_without_lock(order_req);
    return true;
  }

  void on_logon_msg(MessageHeader* header, LogonMessage* msg) override;

  void on_logout_msg(MessageHeader* header, LogoutMessage* msg) override;

  void on_invalid_msg() override;

  // 下面的回调函数处理完后都会去检测消息队列里是否有消息等待消费
  void on_heartbeat_msg(MessageHeader* header, HeartbeatMessage* msg) override;

  void on_test_request(MessageHeader* header, TestRequest* req) override;

  void on_resend_request(MessageHeader* header, ResendRequest* req) override;

  void on_sequence_reset_msg(MessageHeader* header, SequenceResetMessage* msg) override;

  void on_reject_msg(MessageHeader* header, RejectMessage* msg) override;

  void on_business_reject_msg(MessageHeader* header, BusinessRejectMessage* msg) override;

  void on_execution_report(MessageHeader* header, ExecutionReport* msg) override;

  void on_mass_cancel_report(MessageHeader* header, OrderMassCancelReport* msg) override;

  void on_quote_status_report(MessageHeader* header, QuoteStatusReport* msg) override;

  void on_trade_capture_report(MessageHeader* header, TradeCaptureReport* msg) override;

  void on_trade_capture_report_ack(MessageHeader* header, TradeCaptureReportAck* msg) override;

  void disconnect(DisconnectReason reason, bool called_by_sender);

  bool periodically_check();

  std::string comp_id() const { return comp_id_; }

  /*
   * 当前是否允许登录
   *
   * 如果is_enabled为false，表示当前不允许登录，将不会连接到OCG
   * 如果is_enabled为true，则会自动连接或是重连到OCG并自动登录
   * 通过enable和disable可从外部主动控制登录登出
   */
  bool is_enabled() const { return state_.enabled; }

  /*
   * 使session变为允许登录状态
   */
  void enable() {
    std::unique_lock<std::mutex> lock(mutex_);
    state_.enabled = true;
  }

  /*
   * 禁止session登录
   *
   * 如果当前已登录，则会自动登出
   */
  void disable() {
    std::unique_lock<std::mutex> lock(mutex_);
    state_.enabled = false;
  }

 private:
  // 登出，一般情况不会用到，外部不允许主动登出
  // 只有在每个交易日结束或是出现错误需要主动登出时才调用
  void send_logout_msg(BssSessionStatus status = SESSION_STATUS_OTHER,
                       const std::string& text = "");

  // 内部使用的断线函数
  void internal_disconnect(DisconnectReason reason) { disconnect(reason, false); }

  // process函数处理完后不会去处理queue中缓存的数据
  void process_msg(MessageHeader* header, HeartbeatMessage* msg) {
    printf("on_heartbeat. test_req_id:%u\n", msg->reference_test_request_id);

    if (!verify_msg(*header, *msg)) return;

    ++state_.next_recv_msg_seq;
    // 如果心跳消息中有reference_test_request_id，则是对BSS发出的TestRequest的回应
    if (msg->reference_test_request_id > 0) state_.sent_test_request = false;
  }

  /*
   * 当OCG在连续三个heartbeat的时间间隔内都未收到数据，OCG会给BSS发送TestRequest，
   * 此时如果BSS收到该TestRequest，应该立即回复一个heartbeat并将TestRequest中的
   * TestRequestId带回给TestRequest。若OCG在之后连续三个heartbeat的时间间隔内仍
   * 未收到该TestRequest的回应，则会触发logout及连接断开
   */
  void process_msg(MessageHeader* header, TestRequest* req) {
    if (!verify_msg(*header, *req)) return;

    // 发送heartbeat以回应test request，带上test request
    // id以明确是回应哪个request
    send_heartbeat(req->test_request_id);

    ++state_.next_recv_msg_seq;
  }

  void process_msg(MessageHeader* header, ResendRequest* req);

  void process_msg(MessageHeader* header, SequenceResetMessage* msg);

  // Session不处理业务逻辑，故不关心非admin消息的类型
  // 只需要对业务消息统一做完整性及正确性检查即可
  template <class Message>
  void process_msg(MessageHeader* header, Message* msg) {
    if (!verify_msg(*header, *msg)) return;

    broker_->on_msg(*msg);
    ++state_.next_recv_msg_seq;
  }

  template <class Message>
  bool verify_msg(const MessageHeader& header, const Message& msg, bool check_too_high = true,
                  bool check_too_low = true);

  /*
   * 检测消息是否在合适状态下的到来
   */
  bool validate_logon_state(const MessageHeader& header) const {
    return (header.message_type != LOGON && state_.received_logon) ||
           (header.message_type == LOGON && state_.sent_logon && !state_.received_logon) ||
           header.message_type == LOGOUT;
  }

  template <class Message>
  bool handle_msg_seq_too_high(const MessageHeader& header, const Message& msg);

  bool handle_msg_seq_too_low(const MessageHeader& header);

  /*
   * 处理重复的消息
   */
  void handle_poss_dup(const MessageHeader& header) {
    if (header.message_type != SEQUENCE_RESET) {
      // todo: do sth?
    }
  }

  bool is_correct_comp_id(const CompId& comp_id) const { return comp_id_ == comp_id; }

  void resend(uint32_t start_seq_num, uint32_t end_seq_num);

  void consume_cached_msgs();

  // 判断消息缓存（用于存放提前到达的消息）里是否有数据
  bool is_msg_queue_empty() const { return state_.is_msg_queue_empty(); }

  bool is_send_vol_exceed_threshold() const { return current_sec_send_ >= msg_limit_per_sec_; }

  // 不加锁的发送，resend场景下需要在外部加一个大锁，发送函数内部不能有锁
  template <class Message>
  bool send_raw_msg_without_lock(const Message& msg);

  // 除了resend，其余场景发送时均需要加锁
  template <class Message>
  bool send_raw_msg(const Message& msg) {
    std::unique_lock<std::mutex> lock(mutex_);
    return send_raw_msg_without_lock(msg);
  }

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

  void send_resend_request(uint32_t start_seq, uint32_t end_seq) {
    ResendRequest resend_req{start_seq, end_seq};
    send_raw_msg(resend_req);

    state_.set_resend_range(start_seq, end_seq);
    printf("send_resend_request: [%u, %u]\n", start_seq, end_seq);
  }

  void send_reject_msg(const MessageHeader& err_msg_header, RejectCode reject_code,
                       const std::string& err_field_name = "") {
    RejectMessage reject_msg{};
    reject_msg.message_reject_code = reject_code;
    reject_msg.reference_sequence_number = err_msg_header.message_type;
    strncpy(reject_msg.reference_field_name, err_field_name.c_str(), sizeof(ReferenceFieldName));

    send_raw_msg(reject_msg);
  }

  void send_gap_fill(uint32_t new_seq_num) {
    // gap_fill设置为Y表示让OCG跳过该消息，N有别的用途，BSS方只可使用Y功能
    SequenceResetMessage seq_reset_msg{'Y', new_seq_num};
    send_raw_msg_without_lock(seq_reset_msg);
    state_.next_send_msg_seq = new_seq_num;
  }

  bool update_throttle();

  void encrypt_password();

 private:
  struct ResendVisitor {
    void Init(Session* self) { self_ = self; }

    template <class Message>
    void operator()(const Message& msg) {
      self_->send_raw_msg_without_lock(msg);
    }

   private:
    Session* self_;
  };

  struct ConsumerVisitor {
    void Init(Session* self) { self_ = self; }
    void set_header(MessageHeader* header) { header_ = header; }

    void operator()(LogonMessage& msg) { BUG_ON(); }
    void operator()(LogoutMessage& msg) { BUG_ON(); }

    template <class Message>
    void operator()(Message& msg) {
      self_->process_msg(header_, &msg);
    }

   private:
    Session* self_;
    MessageHeader* header_;
  };

 private:
  std::string comp_id_;
  LogonMessage logon_msg_;
  std::string password_;
  std::string new_password_;
  SessionState state_;
  BinaryMessageEncoder encoder_;
  BssBroker* broker_{nullptr};
  SocketSender* socket_sender_{nullptr};
  std::unique_ptr<PasswordEncrptor> passwd_encrypto_;
  std::mutex mutex_;

  std::queue<MsgBuffer> cached_snd_queue_;
  uint32_t current_sec_send_{0};
  uint32_t msg_limit_per_sec_{0};

  ResendVisitor resend_visitor_;
  ConsumerVisitor consumer_visitor_;
};

/*
 * 非预期序列号处理函数
 *
 * 收到从OCG发来的序列号大于预期的消息
 */
template <class Message>
bool Session::handle_msg_seq_too_high(const MessageHeader& header, const Message& msg) {
  // todo: To be or not to be, that is the question
  printf("handle_msg_seq_too_high: seq:%u type:%u expected:%u\n", header.sequence_number,
         header.message_type, state_.next_recv_msg_seq);

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

/*
 * 验证消息是否正确
 *
 * 序列号过高或过低都会有相应的处理，严重情况需要直接断开连接
 * 这里可以选择性是否判断过高或过低，因为登录的时候就无需判断是否过高
 */
template <class Message>
bool Session::verify_msg(const MessageHeader& header, const Message& msg, bool check_too_high,
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

  // 如果BSS之前发出了重传请求，则此处判断重传状态是否结束
  if (state_.is_recovering() && header.sequence_number >= state_.resend_range.second) {
    // ResendRequest has been satisfied
    printf("ResendRequest[%u, %u] has been satisfied\n", state_.resend_range.first,
           state_.resend_range.second);
    state_.set_resend_range(0, 0);
  }

  state_.last_received_time_ms = now_ms();
  printf("pass. seq:%u\n", header.sequence_number);
  return true;

error:
  internal_disconnect(DisconnectReason::INCORRECT_MSG);
  return false;
}

/*
 * 消息发送函数，所有消息最终都通过该入口发送到Connection层
 *
 * 返回true表示成功发送到了Connection层
 * 返回false表示未发送到Connection层
 * 但无论消息是否发送到Connection层，都会被记录在历史消息中
 */
template <class Message>
bool Session::send_raw_msg_without_lock(const Message& msg) {
  bool res = true;
  MsgBuffer msg_buffer;

  encoder_.set_next_seq_number(state_.next_send_msg_seq);
  encoder_.encode_msg(msg, &msg_buffer);

  if (current_sec_send_ >= msg_limit_per_sec_) {
    // 当前秒钟内可流量已达上限，把消息缓存到队列中，由定时器去发出
    cached_snd_queue_.emplace(msg_buffer);
  } else {
    // 这里需要判断指针是否为空，因为在进入send_raw_msg之前可能已经触发了断线
    if (socket_sender_) res = socket_sender_->send(msg_buffer.data, msg_buffer.size);
  }

  // 每条发出去的消息都保存下来，以处理在登录时OCG请求重传的情况
  // 即便socket_sender_为空使得发送不成功，也应该保存该消息
  state_.record_sent_msg(msg);
  state_.last_send_time_ms = now_ms();
  ++state_.next_send_msg_seq;
  ++current_sec_send_;
  return res;
}

}  // namespace ft::bss

#endif  // OCG_BSS_BROKER_SESSION_H_
