// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "broker/session.h"

#include <unistd.h>

#include <cstdio>
#include <cstring>

namespace ft::bss {

Session::Session(BssBroker* broker) : broker_(broker) {
  resend_visitor_.init(this);
  consumer_visitor_.init(this);
}

bool Session::init(const SessionConfig& conf) {
  passwd_encrypto_ = std::make_unique<PasswordEncrptor>(conf.rsa_pubkey_file);

  comp_id_ = conf.comp_id;
  encoder_.set_comp_id(comp_id_);
  set_password(conf.password, conf.new_password);
  msg_limit_per_sec_ = conf.msg_limit_per_sec;

  return true;
}

/*
 * 调用者需要确保密码符合OCG文档对密码安全策略的要求
 */
void Session::set_password(const std::string& passwd,
                           const std::string& new_passwd) {
  password_ = passwd;
  new_password_ = new_passwd;
}

void Session::encrypt_password() {
  memset(logon_msg_.password, 0, sizeof(logon_msg_.password));
  memset(logon_msg_.new_password, 0, sizeof(logon_msg_.new_password));

  tm _tm{};
  auto utc = time(nullptr);
  gmtime_r(&utc, &_tm);

  char buf[64];
  strftime(buf, sizeof(buf), "%Y%m%d%H%M%S", &_tm);
  std::string password = std::string(buf) + password_;
  passwd_encrypto_->encrypt(password.c_str(), password.length(),
                            logon_msg_.password);

  if (!new_password_.empty()) {
    std::string new_password = std::string(buf) + new_password_;
    passwd_encrypto_->encrypt(new_password.c_str(), new_password.length(),
                              logon_msg_.new_password);
  }
}

/*
 * 重置至初始状态
 *
 * 在OCG或是BSS发生严重错误后，由EP告知OCG，或是OCG告知EP需要重置时，
 * 使用该函数进行重置
 */
void Session::reset() {
  state_.reset();
  current_sec_send_ = 0;
  while (!cached_snd_queue_.empty()) cached_snd_queue_.pop();
}

/*
 * 登录回报处理
 *
 * 登录成功只能发生在以下条件都满足的情况下：
 * 1. 连接成功后BSS发出的第一条消息为logon，且在收到回报前没有发出其他消息
 * 2. BSS发出的logon消息内容准确无误，MsgSeqNum大于等于OCG的NextExpectedSeqNum,
 *    BSS的ExpectedSeqNum小于OCG的NextSendSeqNum
 * 3. 发出logon消息后，收到从OCG发来的第一条消息的类型为logon
 * 4. logon回报的CompID正确，MsgSeqNum大于等于BSS的NextExpected，OCG的
 *    NextExpectedSeqNum小于等于BSS的NextSendSeqNum
 */
void Session::on_logon_msg(MessageHeader* header, LogonMessage* msg) {
  // 登录期间，如果登录回应的MsgSeq比BSS期望的高不应该视为错误处理
  // BSS也不必发送ResendRequest请求重传，OCG会自动重传
  if (!verify_msg(*header, *msg, false, true)) return;

  if (msg->next_expected_message_sequence == 0 ||
      msg->next_expected_message_sequence > state_.next_send_msg_seq) {
    // if OCG's ExpectedRecvMsgSeq was greater than BSS's NextSendSeqNum or it's
    // 0, the logon message would be rejected because it is illogical
    internal_disconnect(DisconnectReason::INCORRECT_MSG);
    return;
  }

  if (msg->next_expected_message_sequence < state_.next_send_msg_seq) {
    // if OCG's ExpectedRecvMsgSeq is less than BSS's NextSendMsgSeq,
    // resend from start with pos_dup_flags. Please refer to the appendix A of
    // 'OCG E2E Test User Guide'
    resend(msg->next_expected_message_sequence, 0);
  }

  // 如果OCG回复的登录回报的SeqNum比BSS期望的大，则OCG会自动重传BSS所需要的数据，即
  // [BSS ExpectedRecvSeqNum, ThisMsgSeqNum]之间的数据，此处只是记录一下
  if (header->sequence_number > state_.next_recv_msg_seq)
    state_.set_resend_range(state_.next_recv_msg_seq, header->sequence_number);

  state_.received_logon = true;
  if (state_.next_recv_msg_seq == header->sequence_number)
    ++state_.next_recv_msg_seq;

  printf("[Session::on_logon_msg] MsgSeq:%u Recovery:[%u, %u]\n",
         header->sequence_number, state_.resend_range.first,
         state_.resend_range.second);

  broker_->on_msg(*msg);
}

void Session::on_logout_msg(MessageHeader* header, LogoutMessage* msg) {
  printf("on logout\n");
  // 对于logout，不检查SeqNum，只检查消息的合法性，收到立即登出
  if (!verify_msg(*header, *msg, false, false)) return;

  // 如果logout消息就是下一条待收消息，则递增序列号
  if (header->sequence_number == state_.next_recv_msg_seq)
    ++state_.next_recv_msg_seq;

  broker_->on_msg(*msg);

  if (msg->session_status == SESSION_STATUS_OTHER && msg->logout_text.len > 0) {
    if (header->sequence_number < state_.next_recv_msg_seq) {
      // 如果是本地的待收序列号比对方待发的要大而导致的登出，解析Logout
      // reason得到正确的本地待收序列号，然后断开连接，等待下次重连
      std::string_view reason = kReasonExpectedLargerThanOcgSndSeq;
      auto pos = std::string_view(msg->logout_text.data).find(reason);
      if (pos != std::string_view::npos) {
        state_.next_recv_msg_seq =
            std::stoul(msg->logout_text.data + reason.length());
        printf("NextExpectedSeqNum fixed. Now is %u\n",
               state_.next_recv_msg_seq);
        internal_disconnect(DisconnectReason::LOGOUT);
        return;
      }
    } else {
      // 如果是本地的待发序列号比对方期望的要小而导致的登出，则解析logout
      // reason得到正确的本地待发序列号， 然后直接断开连接，等待下次重连。
      // 出现这种情况应该是BSS因为BUG意外退出然后重启了，使得序列号对不上
      std::string_view reason = kReasonLocalSndSeqLessThanOcgExpected;
      auto pos = std::string_view(msg->logout_text.data).find(reason);
      if (pos != std::string_view::npos) {
        state_.next_send_msg_seq =
            std::stoul(msg->logout_text.data + reason.length());
        printf("NextSndSeqNum fixed. Now is %u\n", state_.next_send_msg_seq);
        internal_disconnect(DisconnectReason::LOGOUT);
        return;
      }
    }
  }

  // 如果是OCG先发出的登出请求，需要回应一个logout
  if (!state_.sent_logout) send_logout_msg();
  internal_disconnect(DisconnectReason::LOGOUT);
}

/*
 * 收到未校验通过的消息，要断开连接，因为数据可能错乱了
 */
void Session::on_invalid_msg() {
  internal_disconnect(DisconnectReason::INCORRECT_MSG);
}

void Session::on_heartbeat_msg(MessageHeader* header, HeartbeatMessage* msg) {
  process_msg(header, msg);
  if (!is_msg_queue_empty()) consume_cached_msgs();
}

void Session::on_test_request(MessageHeader* header, TestRequest* req) {
  process_msg(header, req);
  if (!is_msg_queue_empty()) consume_cached_msgs();
}

void Session::on_resend_request(MessageHeader* header, ResendRequest* req) {
  process_msg(header, req);
  if (!is_msg_queue_empty()) consume_cached_msgs();
}

void Session::on_sequence_reset_msg(MessageHeader* header,
                                    SequenceResetMessage* msg) {
  process_msg(header, msg);
  if (!is_msg_queue_empty()) consume_cached_msgs();
}

void Session::on_reject_msg(MessageHeader* header, RejectMessage* msg) {
  process_msg(header, msg);
  if (!is_msg_queue_empty()) consume_cached_msgs();
}

void Session::on_business_reject_msg(MessageHeader* header,
                                     BusinessRejectMessage* msg) {
  process_msg(header, msg);
  if (!is_msg_queue_empty()) consume_cached_msgs();
}

void Session::on_execution_report(MessageHeader* header, ExecutionReport* msg) {
  process_msg(header, msg);
  if (!is_msg_queue_empty()) consume_cached_msgs();
}

void Session::on_mass_cancel_report(MessageHeader* header,
                                    OrderMassCancelReport* msg) {
  process_msg(header, msg);
  if (!is_msg_queue_empty()) consume_cached_msgs();
}

void Session::on_quote_status_report(MessageHeader* header,
                                     QuoteStatusReport* msg) {
  process_msg(header, msg);
  if (!is_msg_queue_empty()) consume_cached_msgs();
}

void Session::on_trade_capture_report(MessageHeader* header,
                                      TradeCaptureReport* msg) {
  process_msg(header, msg);
  if (!is_msg_queue_empty()) consume_cached_msgs();
}

void Session::on_trade_capture_report_ack(MessageHeader* header,
                                          TradeCaptureReportAck* msg) {
  process_msg(header, msg);
  if (!is_msg_queue_empty()) consume_cached_msgs();
}

void Session::process_msg(MessageHeader* header, ResendRequest* req) {
  if (!verify_msg(*header, *req)) return;

  if (req->start_sequence == 0 ||
      req->start_sequence >= state_.next_send_msg_seq) {
    send_reject_msg(*header, MSG_REJECT_CODE_VALUE_IS_INCORRECT_FOR_THIS_FIELD,
                    "Start Sequence");
    return;
  } else if ((req->end_sequence != 0 &&
              req->end_sequence < req->start_sequence) ||
             req->end_sequence >= state_.next_send_msg_seq) {
    send_reject_msg(*header, MSG_REJECT_CODE_VALUE_IS_INCORRECT_FOR_THIS_FIELD,
                    "End Sequence");
    return;
  }

  resend(req->start_sequence, req->end_sequence);
  ++state_.next_recv_msg_seq;
}

void Session::process_msg(MessageHeader* header, SequenceResetMessage* msg) {
  if (!verify_msg(*header, *msg)) return;

  // 无论是reset或是gap fill，SequenceResetMessage消息中的new_sequence_number
  // 都是让BSS把NextRecvSeqNum置为一个更大的值，如果该值比BSS的NextRecvMsgSeq小，
  // 则是一个非法的SequenceResetMessage，应该回以拒绝
  if (msg->new_sequence_number < state_.next_recv_msg_seq) {
    send_reject_msg(*header, MSG_REJECT_CODE_VALUE_IS_INCORRECT_FOR_THIS_FIELD,
                    "New Sequence Number");
    return;
  }

  printf("[Session::on_seq_reset] current:%u next:%u\n",
         header->sequence_number, msg->new_sequence_number);

  state_.next_recv_msg_seq = msg->new_sequence_number;
  if (state_.is_recovering() &&
      state_.next_recv_msg_seq > state_.resend_range.second) {
    printf("ResendRequest[%u, %u] has been satisfied\n",
           state_.resend_range.first, state_.resend_range.second);
    state_.set_resend_range(0, 0);
  }
}

// "return false" means a fatal error, and outer function should call
// disconnect
bool Session::handle_msg_seq_too_low(const MessageHeader& header) {
  // 收到MsgSeqNum比ExpectedSeqNum小的消息，应该直接断开连接，
  // 如果是登录消息或是其他类型的未设置pos_dup_flag的消息，应该给予对方提示
  if (header.message_type == LOGON || header.poss_dup_flag == 0) {
    printf("failed. msg seq num is too low, received:%u expected:%u\n",
           header.sequence_number, state_.next_recv_msg_seq);
    std::string logout_reason = kReasonLocalSndSeqLessThanOcgExpected;
    logout_reason += std::to_string(state_.next_recv_msg_seq);
    send_logout_msg(SESSION_STATUS_OTHER, logout_reason);
    return false;
  }

  handle_poss_dup(header);
  return true;
}

/*
 * 消息重传
 *
 * 如果因为网络故障导致断线，重连并发出登录消息后，BSS收到从OCG发来的登录回应包。
 * BSS发现本地维护的SendMsgSeq大于OCG回应包中的NextExpectedMsgSeq，则BSS需要向
 * OCG重传从NextExpectedMsgSeq开始的所有消息。如果是LOGON、LOGOUT、HEARTBEAT、
 * TEST_REQUEST、RESEND_REQUEST、SEQUENCE_RESET会话状态类型的消息，应替换为
 * SequenceResetMessage发送给OCG以使其跳过此类消息，其余消息都应该原封不动地传
 * 给OCG。特别要注意的是重传消息的MsgSeqNum应该从NextExpectedMsgSeq往上递增，即
 * 和第一次发送时相同，且PossDupFlag应该设置为'Y'
 *
 * start_seq_num: 重传的起始位置
 * end_seq_num: 重传的结尾位置，包含该消息，如果为0则表示重传到最新的消息
 * 调用者需要确保参数的合法性
 */
void Session::resend(uint32_t start_seq_num, uint32_t end_seq_num) {
  std::unique_lock<std::mutex> lock(mutex_);

  // NextSendSeqNum重置为OCG的NextExpectedSeqNum，并设置PossDupFlag
  uint32_t cur_next_send_seq = state_.next_send_msg_seq;
  state_.next_send_msg_seq = start_seq_num;
  encoder_.set_poss_dup_flag(1);

  if (end_seq_num == 0) end_seq_num = cur_next_send_seq - 1;

  uint32_t seq_reset_target = 0;
  for (uint32_t num = start_seq_num; num <= end_seq_num; ++num) {
    auto iter = state_.sent_messages.find(num);
    if (iter == state_.sent_messages.end()) {
      // 如果没有找到历史记录，应该是个BUG
      // 因为每次都会先把发送的数据记录下来然后才增加本地SeqNum
      BUG_ON();
    }

    const auto& msg = iter->second;
    auto type = msg.type;
    // 如果是下列类型的消息则使用Sequence Reset Message跳过，因为重传这些会话
    // 控制类型的消息并没有意义
    if (type == LOGON || type == LOGOUT || type == HEARTBEAT ||
        type == TEST_REQUEST || type == RESEND_REQUEST ||
        type == SEQUENCE_RESET) {
      // 可用一个seq reset跳过多条消息
      // 此处只是记录下一条消息跳到哪里，并不直接发送seq reset
      if (seq_reset_target == 0)
        seq_reset_target = state_.next_send_msg_seq + 1;
      else
        ++seq_reset_target;
    } else {
      // 如果需要seq reset，这里先把seq reset发出去
      if (seq_reset_target != 0) {
        send_gap_fill(seq_reset_target);
        seq_reset_target = 0;
      }

      // 下列消息体则原封不动地重传
      // todo: 如果断线时间太长，是否应该使用SequenceResetMessage来跳过某些
      //       请求类型消息，比如订单请求，因为这中间市场很大可能发生了很大波
      //       动使得盈利机会消失
      std::visit(resend_visitor_, msg.body);
    }
  }

  if (seq_reset_target != 0) send_gap_fill(seq_reset_target);

  // 至此消息全部重传完毕，NextSendSeqNum已经恢复到该函数调用前的状态
  // 需要重置PossDupFlag
  encoder_.set_poss_resend_flag(0);
  state_.next_send_msg_seq = cur_next_send_seq;
}

/*
 * 尝试消费消息缓存里的数据
 */
void Session::consume_cached_msgs() {
  CachedRecvMessage msg{};
  for (;;) {
    // 从消息缓存中获取下一条待消费的消息，如果下一条消息还未达则直接返回
    if (!state_.retrieve_msg_cache(state_.next_recv_msg_seq, &msg)) break;

    // 根据不同消息类型调用不同的处理函数，std::visit中会回调相应的process_msg
    consumer_visitor_.set_header(&msg.header);
    std::visit(consumer_visitor_, msg.body);
  }
}

void Session::send_logon_msg() {
  logon_msg_.next_expected_message_sequence = state_.next_recv_msg_seq;
  encrypt_password();

  // 这里判断返回值是为了使登录消息未发送成功时能够再一次地发送
  // 因为发送登录消息的时候socket_sender可能还未注册，导致发送失败
  if (send_raw_msg(logon_msg_)) {
    printf("send logon\n");
    state_.last_received_time_ms = now_ms();
    state_.sent_logon = true;
  }
}

void Session::send_logout_msg(BssSessionStatus status,
                              const std::string& text) {
  LogoutMessage logout_msg{};

  logout_msg.session_status = status;
  if (!text.empty() && text.size() < sizeof(logout_msg.logout_text.data)) {
    strncpy(logout_msg.logout_text.data, text.c_str(),
            sizeof(logout_msg.logout_text.data));
    logout_msg.logout_text.len = text.size() + 1;
  }

  send_raw_msg(logout_msg);
  state_.sent_logout = true;
}

/*
 * 主动断线函数，供Connection使用
 *
 * 当出现了异常情况（收到错误的消息等，详情需参考OCG文档），BSS应该主动断开连接
 * 断开连接后本地维护的数据也不应该释放，而是等待重连后继续使用
 *
 * called_by_sender参数说明：
 * 1. disconnect必须运行在锁内，如果disconnect的上下文不在锁内，就需要在
 *    disconnect内部加锁，否则不能加锁，不然会死锁。
 * 2. 不能在内部加锁的场景：发消息的时候因为异常而触发的disconnect
 *    因为send消息的时候外面本身已经加锁，send过程中也可能会触发disconnect，
 *    所以发送方调用disconnect的时候不能加锁，需要把called_by_sender置为true
 */
void Session::disconnect(DisconnectReason reason, bool called_by_sender) {
  if (called_by_sender) {
    if (socket_sender_) {
      socket_sender_->disconnect(reason);
      socket_sender_ = nullptr;

      LogoutMessage logout_msg{};
      logout_msg.session_status = 101;
      broker_->on_msg(logout_msg);
    }

    while (!cached_snd_queue_.empty())
      cached_snd_queue_.pop();  // 清空流速缓存队列
    state_.reset_on_logout();
  } else {
    std::unique_lock<std::mutex> lock(mutex_);
    disconnect(reason, true);
  }
}

/*
 * 更新当前秒钟的订单流量，并尝试把上一秒缓存待发的数据发送出去
 *
 * 该函数会在每个秒钟的开始时被定时器调用，然后执行以下操作：
 * 1. 把current_sec_send(当前秒钟内发出的消息条数)置0
 * 2. 如果缓存队列中有消息，如果把current_sec_send在流速限制范围内，
 *    则把缓存队列中的消息发送出去，否则需要等待下一秒钟才发
 */
bool Session::update_throttle() {
  std::unique_lock<std::mutex> lock(mutex_);
  if (!socket_sender_) return false;

  // current_sec_send_置0后再尝试发送缓存中的消息
  for (current_sec_send_ = 0; current_sec_send_ < msg_limit_per_sec_;
       ++current_sec_send_) {
    if (cached_snd_queue_.empty()) return true;

    auto msgbuf = cached_snd_queue_.front();
    if (!socket_sender_->send(msgbuf.data, msgbuf.size)) return false;
    cached_snd_queue_.pop();
  }

  return true;
}

/*
 * 根据当前会话状态进行一些检查并执行相应操作
 *
 * 1. 如果当前未收到登录回应有两种情况
 *   a. 未发送登录请求，则立即发送一个登录请求
 *   b. 已发送登录请求，未超时则返回，超时立即断开连接，超时时间为1分钟
 * 2. 如果BSS主动发出了登出请求，如果60秒内未收到登出回应，则立即断开连接
 * 3. 是否需要发送心跳（当前离上一次消息发出已经过去了20秒），需要则立即发送
 * 4. 是否需要发送Test（当前离上一次收到消息已经过去了60秒），需要则立即发送
 * 5. 发出Test请求后，是否在60秒超时时间内都未收到相应的回应，是则立即断开连接
 *
 * 返回false表示发生了错误，即连接中断了，外部无需再调用disconnect
 */
bool Session::periodically_check() {
  uint64_t now = now_ms();

  if (!state_.received_logon) {
    if (!state_.sent_logon) {
      send_logon_msg();
    } else if (state_.is_logon_timeout(now)) {
      // 登录请求发出后，一分钟内未收到登录回应则强制断开连接
      internal_disconnect(DisconnectReason::LOGON_TIMEOUT);
      return false;
    }

    return true;
  }

  // 登出消息发出后，如果一分钟内没收到登出回应，则强制断开连接
  if (state_.sent_logout) {
    if (state_.is_logout_timeout(now)) {
      internal_disconnect(DisconnectReason::LOGOUT_TIMEOUT);
      return false;
    }

    return true;
  }

  // 如果当前会话被disable了，则应当登出
  if (!is_enabled()) {
    send_logout_msg();
    return true;
  }

  if (!update_throttle()) return false;

  if (state_.need_heartbeat(now)) send_heartbeat();

  if (state_.need_test_request(now)) send_test_request();

  // x秒未收到TestRequest的心跳包回应
  if (state_.is_test_request_timeout(now)) {
    send_logout_msg();
    internal_disconnect(DisconnectReason::TEST_REQUEST_FAILED);
    return false;
  }

  return true;
}

}  // namespace ft::bss
