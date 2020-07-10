// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstdint>
#include <thread>

#include "protocol/protocol.h"
#include "protocol/protocol_encoder.h"
#include "protocol/protocol_parser.h"
#include "testcase.h"

using namespace ft::bss;

// CompID: CO99999902

class OcgBinaryDecoder {
 public:
  OcgBinaryDecoder() {}

  char* front() { return buffer_; }

  char* back() { return buffer_ + pos_; }

  std::size_t size() const { return pos_; }

  bool pop() {
    if (pos_ < sizeof(MessageHeader) ||
        pos_ < reinterpret_cast<MessageHeader*>(buffer_)->length)
      return false;

    std::size_t len = reinterpret_cast<MessageHeader*>(buffer_)->length;
    std::size_t rest = pos_ - len;
    memmove(buffer_, buffer_ + len, rest);
    pos_ = rest;
    return true;
  }

  std::size_t recv_from(int sock) {
    if (pos_ >= sizeof(MessageHeader) &&
        pos_ >= reinterpret_cast<MessageHeader*>(buffer_)->length)
      return pos_;

    for (;;) {
      auto each = recv(sock, buffer_ + pos_, sizeof(buffer_) - pos_, 0);
      if (each <= 0) exit(-1);
      pos_ += each;
      if (pos_ < sizeof(MessageHeader)) continue;
      if (pos_ < reinterpret_cast<MessageHeader*>(buffer_)->length) continue;
      break;
    }

    return pos_;
  }

 private:
  char buffer_[4096 * 20]{};
  std::size_t pos_{0};
};

OcgBinaryDecoder decoder;
BinaryMessageEncoder encoder;
int sock;

bool connect_to_ocg(const std::string& ip, int port) {
  sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock < 0) {
    return false;
  }

  sockaddr_in addrin{};
  if (inet_pton(AF_INET, ip.c_str(), &addrin.sin_addr) <= 0) {
    return false;
  }
  addrin.sin_family = AF_INET;
  addrin.sin_port = htons(static_cast<int16_t>(port));

  return connect(sock, (sockaddr*)(&addrin), sizeof(addrin)) == 0;
}

template <class Message>
void send_msg(const Message& msg) {
  MsgBuffer msg_buffer;
  encoder.encode_msg(msg, &msg_buffer);
  send(sock, msg_buffer.data, msg_buffer.size, 0);
}

void run_testcase(const NewOrderRequest& testcase, int reports) {
  sleep(3);

  send_msg(testcase);
  for (int i = 0; i < reports; ++i) {
    decoder.recv_from(sock);
    decoder.pop();
  }
}

bool logon() {
  send_msg(logon_msg);

  decoder.recv_from(sock);
  if (reinterpret_cast<MessageHeader*>(decoder.front())->message_type ==
      LOGOUT) {
    LogoutMessage msg{};
    parse_logout_message(*reinterpret_cast<MessageHeader*>(decoder.front()),
                         decoder.front() + sizeof(MessageHeader), &msg);
  } else {
    LogonMessage msg{};
    parse_logon_message(*reinterpret_cast<MessageHeader*>(decoder.front()),
                        decoder.front() + sizeof(MessageHeader), &msg);
  }

  decoder.pop();

  return true;
}

void run_testcase_22() {
  sleep(3);

  send_msg(tc_om_22_s1);
  decoder.recv_from(sock);
  decoder.pop();

  send_msg(tc_om_22_s2);
  decoder.recv_from(sock);
  decoder.pop();

  sleep(1);

  send_msg(tc_om_22_s3);
  decoder.recv_from(sock);
  decoder.pop();
}

void run_testcase_23() {
  sleep(3);

  send_msg(tc_om_23_s1);
  decoder.recv_from(sock);
  decoder.pop();

  send_msg(tc_om_23_s2);
  decoder.recv_from(sock);
  decoder.pop();
}

void run_testcase_24() {
  sleep(3);

  send_msg(tc_om_24_s1);
  decoder.recv_from(sock);
  decoder.pop();

  send_msg(tc_om_24_s2);
  decoder.recv_from(sock);
  decoder.pop();

  sleep(1);

  send_msg(tc_om_24_s3);
  decoder.recv_from(sock);
  decoder.pop();
  decoder.recv_from(sock);
  decoder.pop();
}

void run_testcase_25() {
  sleep(3);

  send_msg(tc_om_25);
  decoder.recv_from(sock);
  decoder.pop();
}

void run_testcase_26() {
  sleep(3);

  send_msg(tc_om_26_s1);
  decoder.recv_from(sock);
  decoder.pop();

  send_msg(tc_om_26_s2);
  decoder.recv_from(sock);
  decoder.pop();

  sleep(1);

  send_msg(tc_om_26_s3);
  decoder.recv_from(sock);
  decoder.pop();
  decoder.recv_from(sock);
  decoder.pop();
  decoder.recv_from(sock);
  decoder.pop();
}

void run_testcase_27() {
  sleep(3);

  send_msg(tc_om_27_s1);
  decoder.recv_from(sock);
  decoder.pop();

  send_msg(tc_om_27_s2);
  decoder.recv_from(sock);
  decoder.pop();

  sleep(1);

  send_msg(tc_om_27_s3);
  decoder.recv_from(sock);
  decoder.pop();
  decoder.recv_from(sock);
  decoder.pop();
  decoder.recv_from(sock);
  decoder.pop();
}

void run_testcase_28() {
  sleep(3);

  send_msg(tc_om_28_s1);
  decoder.recv_from(sock);
  decoder.pop();

  send_msg(tc_om_28_s2);
  decoder.recv_from(sock);
  decoder.pop();

  sleep(1);

  send_msg(tc_om_28_s3);
  decoder.recv_from(sock);
  decoder.pop();
  decoder.recv_from(sock);
  decoder.pop();
  decoder.recv_from(sock);
  decoder.pop();
}

void run_testcase_29() {
  sleep(3);

  send_msg(tc_om_29);
  decoder.recv_from(sock);
  decoder.pop();
}

void run_testcase_30() {
  sleep(3);

  send_msg(tc_om_30_s1);
  decoder.recv_from(sock);
  ExecutionReport report{};
  parse_execution_report(*reinterpret_cast<MessageHeader*>(decoder.front()),
                         decoder.front() + sizeof(MessageHeader), &report);
  decoder.pop();

  strncpy(tc_om_30_s2.order_id, report.order_id, sizeof(OrderId));
  send_msg(tc_om_30_s2);
}

void run_testcase_31() {
  sleep(3);

  send_msg(tc_om_31_s1);
  decoder.recv_from(sock);
  decoder.pop();

  send_msg(tc_om_31_s2);
  decoder.recv_from(sock);
  decoder.pop();

  sleep(1);

  send_msg(tc_om_31_s3);
  decoder.recv_from(sock);
  decoder.pop();
}

void run_testcase_32() {
  sleep(3);

  send_msg(tc_om_32_s1);
  decoder.recv_from(sock);
  ExecutionReport report{};
  parse_execution_report(*reinterpret_cast<MessageHeader*>(decoder.front()),
                         decoder.front() + sizeof(MessageHeader), &report);
  decoder.pop();

  strncpy(tc_om_32_s2.order_id, report.order_id, sizeof(OrderId));
  send_msg(tc_om_32_s2);
  decoder.recv_from(sock);
  decoder.pop();
}

void run_testcase_33() {
  sleep(3);

  send_msg(tc_om_33);
  decoder.recv_from(sock);
  decoder.pop();
}

void run_tc_qm_01() {
  sleep(3);

  send_msg(tc_qm_01_s1);
  decoder.recv_from(sock);
  decoder.pop();
  decoder.recv_from(sock);
  decoder.pop();

  send_msg(tc_qm_01_s2);
  decoder.recv_from(sock);
  decoder.pop();
  decoder.recv_from(sock);
  decoder.pop();

  sleep(1);

  send_msg(tc_qm_01_s3);
  decoder.recv_from(sock);
  decoder.pop();
  decoder.recv_from(sock);
  decoder.pop();
  decoder.recv_from(sock);
  decoder.pop();
}

void run_tc_qm_02() {
  sleep(3);

  send_msg(tc_qm_02);
  decoder.recv_from(sock);
  decoder.pop();
}

void run_tc_qm_03() {
  sleep(3);

  send_msg(tc_qm_03_s1);
  decoder.recv_from(sock);
  decoder.pop();
  decoder.recv_from(sock);
  decoder.pop();

  send_msg(tc_qm_03_s2);
  decoder.recv_from(sock);
  decoder.pop();
  decoder.recv_from(sock);
  decoder.pop();
}

void run_tc_qm_04() {
  sleep(3);

  send_msg(tc_qm_04);
  decoder.recv_from(sock);
  decoder.pop();
}

void run_tc_tm_01() {
  sleep(3);

  send_msg(tc_tm_01);
  decoder.recv_from(sock);
  decoder.pop();
}

void run_tc_tm_02() {
  sleep(3);

  send_msg(tc_tm_02);
  decoder.recv_from(sock);
  decoder.pop();
}

void run_tc_tm_03() {
  decoder.recv_from(sock);
  TradeCaptureReport report{};
  parse_trade_capture_report(*reinterpret_cast<MessageHeader*>(decoder.front()),
                             decoder.front() + sizeof(MessageHeader), &report);
  decoder.pop();

  strncpy(report.trade_report_id, "202", sizeof(TradeReportId));
  report.trade_report_type = 6;
  report.trade_handling_instructions = 1;
  strncpy(report.broker_location_id, "aaa", sizeof(BrokerLocationId));
  strncpy(report.transaction_time, "aaa", sizeof(TransactionTime));

  send_msg(report);
  decoder.recv_from(sock);
  decoder.pop();
}

void run_tc_in_01() {
  sleep(3);

  send_msg(tc_in_01_s1);
  decoder.recv_from(sock);
  decoder.pop();

  send_msg(tc_in_01_s2);
  decoder.recv_from(sock);
  decoder.pop();

  send_msg(tc_in_01_s3);
  decoder.recv_from(sock);
  decoder.pop();
}

void run_tc_pe_01() {
  sleep(3);

  PartyEntitlementRequest req{};
  strncpy(req.entitlement_request_id, "800101", sizeof(EntitlementRequestId));

  send_msg(req);
  decoder.recv_from(sock);
  decoder.pop();
  decoder.recv_from(sock);
  decoder.pop();
}

void run_testcase_tc_ol_01() {
  sleep(3);

  send_msg(tc_ol_01_s1);
  decoder.recv_from(sock);
  decoder.pop();

  send_msg(tc_ol_01_s2);
  decoder.recv_from(sock);
  decoder.pop();

  sleep(1);

  send_msg(tc_ol_01_s3);
  decoder.recv_from(sock);
  decoder.pop();

  send_msg(tc_ol_01_s4);
  decoder.recv_from(sock);
  decoder.pop();

  sleep(1);

  send_msg(tc_ol_01_s5);
  decoder.recv_from(sock);
  decoder.pop();

  send_msg(tc_ol_01_s6);
  decoder.recv_from(sock);
  decoder.pop();
  decoder.recv_from(sock);
  decoder.pop();
}

void run_testcase_tc_ol_02() {
  sleep(3);

  send_msg(tc_ol_02);
  decoder.recv_from(sock);
  decoder.pop();
  decoder.recv_from(sock);
  decoder.pop();
  decoder.recv_from(sock);
  decoder.pop();
}

void run_testcase_tc_ol_03() {
  sleep(3);

  send_msg(tc_ol_03_s1);
  decoder.recv_from(sock);
  ExecutionReport report{};
  parse_execution_report(*reinterpret_cast<MessageHeader*>(decoder.front()),
                         decoder.front() + sizeof(MessageHeader), &report);
  decoder.pop();

  strncpy(tc_ol_03_s2.order_id, report.order_id, sizeof(OrderId));
  send_msg(tc_ol_03_s2);
  decoder.recv_from(sock);
  decoder.pop();
  decoder.recv_from(sock);
  decoder.pop();
  decoder.recv_from(sock);
  decoder.pop();
  decoder.recv_from(sock);
  decoder.pop();
}

void run_testcase_tc_ol_04() {
  sleep(3);

  send_msg(tc_ol_04_s1);
  decoder.recv_from(sock);
  ExecutionReport report{};
  parse_execution_report(*reinterpret_cast<MessageHeader*>(decoder.front()),
                         decoder.front() + sizeof(MessageHeader), &report);
  decoder.pop();

  strncpy(tc_ol_04_s2.order_id, report.order_id, sizeof(OrderId));
  send_msg(tc_ol_04_s2);
  decoder.recv_from(sock);
  decoder.pop();
  decoder.recv_from(sock);
  decoder.pop();
  decoder.recv_from(sock);
  decoder.pop();
  decoder.recv_from(sock);
  decoder.pop();
}

void run_testcase_tc_ol_05() {
  sleep(3);

  send_msg(tc_ol_05_s1);
  decoder.recv_from(sock);
  decoder.pop();

  send_msg(tc_ol_05_s2);
  decoder.recv_from(sock);
  decoder.pop();
}

void run_testcase_tc_ol_06() {
  sleep(3);

  send_msg(tc_ol_06_s1);
  decoder.recv_from(sock);
  decoder.pop();

  send_msg(tc_ol_06_s2);
  decoder.recv_from(sock);
  decoder.pop();
}

void testcase() {
  if (!connect_to_ocg("0.0.0.0", 18765)) return;

  if (!logon()) return;

  run_testcase(tc_om_1, 1);     // pass
  run_testcase(tc_om_2, 1);     // pass
  run_testcase(tc_om_3, 1);     // pass
  run_testcase(tc_om_4, 1);     // pass
  run_testcase(tc_om_5, 1);     // pass
  run_testcase(tc_om_6, 1);     // pass
  run_testcase(tc_om_7, 1);     // pass
  run_testcase(tc_om_8, 2);     // pass
  run_testcase(tc_om_9, 3);     // pass
  run_testcase(tc_om_10, 2);    // pass
  run_testcase(tc_om_11, 1);    // pass
  run_testcase(tc_om_12, 2);    // pass
  run_testcase(tc_om_13, 2);    // pass
  run_testcase(tc_om_14, 2);    // pass
  run_testcase(tc_om_15, 1);    // pass
  run_testcase(tc_om_16, 3);    // pass
  run_testcase(tc_om_17, 4);    // pass
  run_testcase(tc_om_17_2, 2);  // pass
  run_testcase(tc_om_18, 1);    // pass
  run_testcase(tc_om_19, 1);    // pass
  run_testcase(tc_om_20, 1);    // pass
  run_testcase(tc_om_21, 1);    // pass
  run_testcase_22();            // pass
  run_testcase_23();            // pass
  run_testcase_24();            // pass
  run_testcase_25();            // pass
  run_testcase_26();            // pass
  run_testcase_27();            // pass
  run_testcase_28();            // pass
  run_testcase_29();            // pass
  run_testcase_30();            // pass
  run_testcase_31();            // pass
  run_testcase_32();            // pass
  run_testcase_33();            // pass

  run_tc_qm_01();  // pass
  run_tc_qm_02();  // pass
  run_tc_qm_03();  // pass
  run_tc_qm_04();  // pass

  run_tc_tm_01();  // pass
  run_tc_tm_02();  // pass
  run_tc_tm_03();  // pass

  run_tc_in_01();  // pass

  run_tc_pe_01();  // pass

  run_testcase_tc_ol_01();  // pass
  run_testcase_tc_ol_02();  // pass
  run_testcase_tc_ol_03();  // pass
  run_testcase_tc_ol_04();  // pass
  run_testcase_tc_ol_05();  // pass
  run_testcase_tc_ol_06();  // pass

  sleep(5);
}

int main() {
  encoder.set_comp_id("CO99999902");
  encoder.set_next_seq_number(1);

  testcase();
}
