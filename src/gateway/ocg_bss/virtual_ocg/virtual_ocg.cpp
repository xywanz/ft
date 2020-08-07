// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "virtual_ocg/virtual_ocg.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/poll.h>
#include <sys/timerfd.h>
#include <unistd.h>

#include <cassert>
#include <cerrno>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <ctime>

#include "virtual_ocg/testcase.h"

using namespace ft::bss;

void VirtualOcg::init() {
  // time_t t = wqutil::getWallTime();
  time_t t = time(nullptr);
  struct tm* _tm = localtime(&t);
  snprintf(date_, sizeof(date_), "%04d%02d%02d", _tm->tm_mday + 1900,
           _tm->tm_mon + 1, _tm->tm_mday);

  encoder_.set_comp_id(comp_id_);
  parser_.set_handler(this);
  resend_visitor_.init(this);
  consumer_visitor_.init(this);

  timerfd_ = timerfd_create(CLOCK_MONOTONIC, 0);
  assert(timerfd_ >= 0);
}

void VirtualOcg::listen(int primary_port, int secondary_port) {
  int servfd;
  int port = primary_port;

again:
  servfd = socket(AF_INET, SOCK_STREAM, 0);
  assert(servfd > 0);

  int opt = 1;
  setsockopt(servfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(int));

  sockaddr_in addrin{};
  addrin.sin_family = AF_INET;
  addrin.sin_addr.s_addr = INADDR_ANY;
  addrin.sin_port = htons(port);

  int res = bind(servfd, reinterpret_cast<sockaddr*>(&addrin), sizeof(addrin));
  if (res != 0) abort();

  res = ::listen(servfd, 5);
  if (res != 0) abort();

  if (servfd_primary_ < 0) {
    printf("create primary socket: %d\n", servfd);
    servfd_primary_ = servfd;
    port = secondary_port;
    goto again;
  } else {
    printf("create secondary socket: %d\n", servfd);
    servfd_secondary_ = servfd;
  }
}

void VirtualOcg::accept() {
  sockaddr cliaddr{};
  socklen_t socklen = sizeof(cliaddr);
  pollfd pfds[2]{};

  pfds[0].fd = servfd_primary_;
  pfds[0].events = POLLIN;
  pfds[1].fd = servfd_secondary_;
  pfds[1].events = POLLIN;

restart:
  int nfds = poll(pfds, 2, -1);
  if (nfds == 0) goto restart;
  if (nfds < 0) {
    if (nfds == EINTR) goto restart;
    abort();
  }

  bool is_primary = pfds[0].revents ? true : false;
  int servfd = is_primary ? servfd_primary_ : servfd_secondary_;
again:
  int sockfd = ::accept(servfd, &cliaddr, &socklen);
  if (sockfd < 0) {
    if (errno == EINTR) goto again;
    printf("servfd:%d  %s\n", servfd, strerror(errno));
    abort();
  } else {
    sockfd_ = sockfd;
    state_.last_received_time_ms = now_ms();
    printf("bss connected to %s server\n",
           is_primary ? "primary" : "secondary");
  }
}

void VirtualOcg::run() {
  pollfd pfds[2]{};

  pfds[0].fd = sockfd_;
  pfds[0].events = POLLIN;
  pfds[1].fd = timerfd_;
  pfds[1].events = POLLIN;

  itimerspec ts{};
  ts.it_interval.tv_sec = 1;
  ts.it_value.tv_sec = 1;
  timerfd_settime(timerfd_, 0, &ts, nullptr);

  for (;;) {
    int nfds = poll(pfds, 2, -1);
    if (nfds == 0) continue;
    if (nfds < 0) {
      if (errno == EINTR) continue;
      abort();
    }

    if (pfds[0].revents != 0) {
      auto res = recv(sockfd_, parser_.writable_start(), parser_.writable(), 0);
      if (res < 0) {
        if (errno == EINTR) continue;
        printf("disconnect\n");
        disconnect();
        break;
      }
      if (res == 0) {
        printf("disconnect\n");
        disconnect();
        break;
      }
      parser_.parse_raw_data(res);
      if (should_disconnect_) {
        printf("disconnect\n");
        break;
      }
    }

    if (pfds[1].revents != 0) {
      uint64_t tmp;
      read(timerfd_, &tmp, 8);
      periodically_check();
      if (should_disconnect_) {
        printf("disconnect\n");
        break;
      }
    }
  }

  close(sockfd_);
  should_disconnect_ = false;
  parser_.clear();
}

void VirtualOcg::send_logon_msg() {
  LogonMessage logon_msg{};

  comp_id_ = "CO99999902";
  encoder_.set_comp_id(comp_id_);

  strncpy(logon_msg.password, "12345", sizeof(logon_msg.password));
  logon_msg.next_expected_message_sequence = state_.next_recv_msg_seq;

  send_raw_msg(logon_msg);
  state_.last_received_time_ms = now_ms();
  state_.enabled = true;
  state_.sent_logon = true;
}

void VirtualOcg::send_logout_msg(BssSessionStatus status,
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

void VirtualOcg::periodically_check() {
  uint64_t now = now_ms();

  if (!state_.received_logon) {
    if (state_.is_logon_timeout(now)) disconnect();
    return;
  }

  if (state_.sent_logout) {
    if (state_.is_logout_timeout(now)) {
      disconnect();
    }

    return;
  }

  if (state_.need_heartbeat(now)) send_heartbeat();

  if (state_.need_test_request(now)) send_test_request();

  // x秒未收到TestRequest的心跳包回应
  if (state_.is_test_request_timeout(now)) disconnect();
}

void VirtualOcg::resend(uint32_t start_seq_num, uint32_t end_seq_num) {
  // NextSendSeqNum重置为OCG的NextExpectedSeqNum，并设置PossDupFlag
  uint32_t cur_next_send_seq = state_.next_send_msg_seq;
  state_.next_send_msg_seq = start_seq_num;
  encoder_.set_poss_dup_flag(1);

  if (end_seq_num == 0) end_seq_num = cur_next_send_seq - 1;
  printf("resend[%u, %u] cur_next_snd:%u\n", start_seq_num, end_seq_num,
         cur_next_send_seq);

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

void VirtualOcg::consume_cached_msgs() {
  OcgCachedRecvMessage msg{};
  for (;;) {
    // 从消息缓存中获取下一条待消费的消息，如果下一条消息还未达则直接返回
    if (!state_.retrieve_msg_cache(state_.next_recv_msg_seq, &msg)) break;

    // 根据不同消息类型调用不同的处理函数，std::visit中会回调相应的process_msg
    consumer_visitor_.set_header(&msg.header);
    std::visit(consumer_visitor_, msg.body);
  }
}

void VirtualOcg::on_logon_msg(MessageHeader* header, LogonMessage* msg) {
  process_msg(header, msg);
}

void VirtualOcg::on_logout_msg(MessageHeader* header, LogoutMessage* msg) {
  process_msg(header, msg);
}

void VirtualOcg::on_heartbeat_msg(MessageHeader* header,
                                  HeartbeatMessage* msg) {
  process_msg(header, msg);
  if (!is_msg_queue_empty()) consume_cached_msgs();
}

void VirtualOcg::on_test_request(MessageHeader* header, TestRequest* msg) {
  process_msg(header, msg);
  if (!is_msg_queue_empty()) consume_cached_msgs();
}

void VirtualOcg::on_resend_request(MessageHeader* header, ResendRequest* msg) {
  process_msg(header, msg);
  if (!is_msg_queue_empty()) consume_cached_msgs();
}

void VirtualOcg::on_reject_msg(MessageHeader* header, RejectMessage* msg) {
  process_msg(header, msg);
  if (!is_msg_queue_empty()) consume_cached_msgs();
}

void VirtualOcg::on_sequence_reset_msg(MessageHeader* header,
                                       SequenceResetMessage* msg) {
  process_msg(header, msg);
  if (!is_msg_queue_empty()) consume_cached_msgs();
}

void VirtualOcg::on_new_order_request(MessageHeader* header,
                                      NewOrderRequest* msg) {
  process_msg(header, msg);
  if (!is_msg_queue_empty()) consume_cached_msgs();
}

void VirtualOcg::on_amend_request(MessageHeader* header, AmendRequest* msg) {
  process_msg(header, msg);
  if (!is_msg_queue_empty()) consume_cached_msgs();
}

void VirtualOcg::on_cancel_request(MessageHeader* header, CancelRequest* msg) {
  process_msg(header, msg);
  if (!is_msg_queue_empty()) consume_cached_msgs();
}

void VirtualOcg::on_mass_cancel_request(MessageHeader* header,
                                        MassCancelRequest* msg) {
  process_msg(header, msg);
  if (!is_msg_queue_empty()) consume_cached_msgs();
}

void VirtualOcg::on_obo_cancel_request(MessageHeader* header,
                                       OboCancelRequest* msg) {
  process_msg(header, msg);
  if (!is_msg_queue_empty()) consume_cached_msgs();
}

void VirtualOcg::on_obo_mass_cancel_request(MessageHeader* header,
                                            OboMassCancelRequest* msg) {
  process_msg(header, msg);
  if (!is_msg_queue_empty()) consume_cached_msgs();
}

void VirtualOcg::on_quote_request(MessageHeader* header, QuoteRequest* msg) {
  process_msg(header, msg);
  if (!is_msg_queue_empty()) consume_cached_msgs();
}

void VirtualOcg::on_quote_cancel_request(MessageHeader* header,
                                         QuoteCancelRequest* msg) {
  process_msg(header, msg);
  if (!is_msg_queue_empty()) consume_cached_msgs();
}

void VirtualOcg::on_quote_cancel_request(MessageHeader* header,
                                         TradeCaptureReport* msg) {
  process_msg(header, msg);
  if (!is_msg_queue_empty()) consume_cached_msgs();
}

void VirtualOcg::on_party_entitlement_request(MessageHeader* header,
                                              PartyEntitlementRequest* msg) {
  process_msg(header, msg);
  if (!is_msg_queue_empty()) consume_cached_msgs();
}

void VirtualOcg::on_invalid_msg() { disconnect(); }

void VirtualOcg::process_msg(MessageHeader* header, LogonMessage* msg) {
  printf("recv logon msg. CompID:%s MsgSeq:%u NextExepected:%u\n",
         header->comp_id, header->sequence_number,
         msg->next_expected_message_sequence);

  if (!verify_msg(*header, *msg, false, true)) return;

  state_.received_logon = true;
  if (state_.next_recv_msg_seq == header->sequence_number)
    ++state_.next_recv_msg_seq;

  uint32_t next_send_msg_seq = state_.next_send_msg_seq;
  if (msg->next_expected_message_sequence < next_send_msg_seq) {
    send_logon_msg();
    resend(msg->next_expected_message_sequence, 0);
    return;
  } else if (msg->next_expected_message_sequence > next_send_msg_seq) {
    printf("BSS's next expected msg seq > OCG's next snd msg seq\n");
    std::string logout_reason = kReasonExpectedLargerThanOcgSndSeq;
    logout_reason += std::to_string(state_.next_send_msg_seq + 1);
    send_logout_msg(SESSION_STATUS_OTHER, logout_reason);
    disconnect();
    return;
  }

  send_logon_msg();
  printf("logon successfully\n");
}

void VirtualOcg::process_msg(MessageHeader* header, LogoutMessage* msg) {
  if (!verify_msg(*header, *msg, false, false)) return;

  if (header->sequence_number == state_.next_recv_msg_seq)
    ++state_.next_recv_msg_seq;

  if (msg->session_status == SESSION_STATUS_OTHER && msg->logout_text.len > 0) {
    std::string_view reason = "Sequence number is less than expected : ";
    auto pos = std::string_view(msg->logout_text.data).find(reason);
    if (pos != std::string_view::npos) {
      state_.next_send_msg_seq =
          std::stoul(msg->logout_text.data + reason.length());
      printf("NextSndSeqNum fixed. Now is %u\n", state_.next_send_msg_seq);
      disconnect();
      return;
    }
  }

  if (!state_.sent_logout) send_logout_msg();
  disconnect();
}

void VirtualOcg::process_msg(MessageHeader* header, HeartbeatMessage* msg) {
  if (!verify_msg(*header, *msg)) return;

  ++state_.next_recv_msg_seq;

  auto now = time(nullptr);
  auto local_time = localtime(&now);
  printf("on_heartbeat. %s", asctime(local_time));
}

void VirtualOcg::process_msg(MessageHeader* header, TestRequest* msg) {
  if (!verify_msg(*header, *msg)) return;

  ++state_.next_recv_msg_seq;

  auto now = time(nullptr);
  auto local_time = localtime(&now);
  printf("on_test_request. test_req_id:%u  time:%s", msg->test_request_id,
         asctime(local_time));

  send_raw_msg(HeartbeatMessage{msg->test_request_id});
}

void VirtualOcg::process_msg(MessageHeader* header, ResendRequest* msg) {
  if (!verify_msg(*header, *msg)) return;

  if (msg->start_sequence == 0 ||
      msg->start_sequence >= state_.next_send_msg_seq) {
    send_reject_msg(*header, MSG_REJECT_CODE_VALUE_IS_INCORRECT_FOR_THIS_FIELD,
                    "Start Sequence");
    return;
  } else if ((msg->end_sequence != 0 &&
              msg->end_sequence < msg->start_sequence) ||
             msg->end_sequence >= state_.next_send_msg_seq) {
    send_reject_msg(*header, MSG_REJECT_CODE_VALUE_IS_INCORRECT_FOR_THIS_FIELD,
                    "End Sequence");
    return;
  }

  resend(msg->start_sequence, msg->end_sequence);
  ++state_.next_recv_msg_seq;
}

void VirtualOcg::process_msg(MessageHeader* header, RejectMessage* msg) {
  if (!verify_msg(*header, *msg)) return;
  printf("recv reject message\n");

  ++state_.next_recv_msg_seq;
}

void VirtualOcg::process_msg(MessageHeader* header, SequenceResetMessage* msg) {
  if (!verify_msg(*header, *msg)) return;

  if (msg->new_sequence_number < state_.next_recv_msg_seq) {
    send_reject_msg(*header, MSG_REJECT_CODE_VALUE_IS_INCORRECT_FOR_THIS_FIELD,
                    "New Sequence Number");
    return;
  }

  state_.next_recv_msg_seq = msg->new_sequence_number;

  printf("on_seq_reset: current:%u next:%u\n", header->sequence_number,
         msg->new_sequence_number);
}

void VirtualOcg::process_msg(MessageHeader* header, NewOrderRequest* msg) {
  if (!verify_msg(*header, *msg)) return;
  printf("recv new order req\n");

  ++state_.next_recv_msg_seq;

  auto reports = tc_accepted_and_several_traded(*msg);
  for (auto report : reports) send_raw_msg(report);
}

void VirtualOcg::process_msg(MessageHeader* header, AmendRequest* msg) {
  if (!verify_msg(*header, *msg)) return;
  printf("recv amend request\n");

  ++state_.next_recv_msg_seq;
}

void VirtualOcg::process_msg(MessageHeader* header, CancelRequest* msg) {
  if (!verify_msg(*header, *msg)) return;
  printf("recv cancel request\n");

  ++state_.next_recv_msg_seq;
}

void VirtualOcg::process_msg(MessageHeader* header, MassCancelRequest* msg) {
  if (!verify_msg(*header, *msg)) return;
  printf("recv mass cancel request\n");

  ++state_.next_recv_msg_seq;
}

void VirtualOcg::process_msg(MessageHeader* header, OboCancelRequest* msg) {
  if (!verify_msg(*header, *msg)) return;
  printf("recv obo cancel request\n");

  ++state_.next_recv_msg_seq;
}

void VirtualOcg::process_msg(MessageHeader* header, OboMassCancelRequest* msg) {
  if (!verify_msg(*header, *msg)) return;
  printf("recv obo mass cancel request\n");

  ++state_.next_recv_msg_seq;
}

void VirtualOcg::process_msg(MessageHeader* header, QuoteRequest* msg) {
  if (!verify_msg(*header, *msg)) return;
  printf("recv quote request\n");

  ++state_.next_recv_msg_seq;
}

void VirtualOcg::process_msg(MessageHeader* header, QuoteCancelRequest* msg) {
  if (!verify_msg(*header, *msg)) return;
  printf("recv quote cancel request\n");

  ++state_.next_recv_msg_seq;
}

void VirtualOcg::process_msg(MessageHeader* header, TradeCaptureReport* msg) {
  if (!verify_msg(*header, *msg)) return;
  printf("recv trade caputre report\n");

  ++state_.next_recv_msg_seq;
}

void VirtualOcg::process_msg(MessageHeader* header,
                             PartyEntitlementRequest* msg) {
  if (!verify_msg(*header, *msg)) return;
  printf("recv party entitlement request\n");

  ++state_.next_recv_msg_seq;
}
