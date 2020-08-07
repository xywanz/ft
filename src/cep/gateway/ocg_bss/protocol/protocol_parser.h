// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef OCG_BSS_PROTOCOL_PARSER_H_
#define OCG_BSS_PROTOCOL_PARSER_H_

#include <cassert>
#include <cstring>
#include <utility>
#include <vector>

#include "protocol/crc32c.h"
#include "protocol/message_handler.h"
#include "protocol/protocol.h"

namespace ft::bss {

class BinaryMessageDecoder {
 public:
  BinaryMessageDecoder();

  void set_handler(MessageHandler* handler);

  int parse_raw_data(std::size_t new_data_size);

  std::size_t readable_size() const { return write_head_ - read_head_; }

  std::size_t writable_size() const { return buffer_.size() - write_head_; }

  char* writable_start() { return buffer_.data() + write_head_; }

  char* readable_start() { return buffer_.data() + read_head_; }

 private:
  std::vector<char> buffer_;
  std::size_t read_head_ = 0;
  std::size_t write_head_ = 0;

  MessageHandler* handler_ = nullptr;
};

template <class NumericType,
          std::enable_if_t<std::is_arithmetic_v<NumericType>, int> = 0>
const char* decode(const char* p, NumericType& field) {
  field = *reinterpret_cast<const NumericType*>(p);
  return p + sizeof(NumericType);
}

template <class CharArray,
          std::enable_if_t<std::is_array_v<CharArray>, int> = 0>
const char* decode(const char* p, CharArray& array) {
  memcpy(array, p, sizeof(CharArray));
  return p + sizeof(CharArray);
}

template <std::size_t N>
const char* decode(const char* p, AlphanumericVariableLength<N>& field) {
  p = decode(p, field.len);
  strncpy(field.data, p, N);
  return p + field.len;
}

inline bool crc32c_check(const MessageHeader* header) {
  auto p = reinterpret_cast<const uint8_t*>(header);
  Checksum checksum =
      crc32c(0xffffffffUL, p, header->length - sizeof(Checksum));
  Checksum trailer =
      *reinterpret_cast<const Checksum*>(p + header->length - sizeof(Checksum));

  return checksum == trailer;
}

inline bool is_present(const PresenceBitmap bitmap, uint32_t i) {
  assert(i < sizeof(PresenceBitmap) * 8);
  return bitmap[i >> 3] & (0b10000000U >> (i & 0x7U));
}

const char* parse_lookup_response(const MessageHeader& header, const char* p,
                                  LookupResponse* rsp);

const char* parse_logon_message(const MessageHeader& header, const char* p,
                                LogonMessage* msg);

const char* parse_logout_message(const MessageHeader& header, const char* p,
                                 LogoutMessage* msg);

const char* parse_heartbeat_message(const MessageHeader& header, const char* p,
                                    HeartbeatMessage* msg);

const char* parse_test_request(const MessageHeader& header, const char* p,
                               TestRequest* msg);

const char* parse_resend_request(const MessageHeader& header, const char* p,
                                 ResendRequest* msg);

const char* parse_reject_message(const MessageHeader& header, const char* p,
                                 RejectMessage* msg);

const char* parse_sequence_reset_message(const MessageHeader& header,
                                         const char* p,
                                         SequenceResetMessage* msg);

const char* parse_execution_report(const MessageHeader& header, const char* p,
                                   ExecutionReport* report);

const char* parse_mass_cancel_report(const MessageHeader& header, const char* p,
                                     OrderMassCancelReport* report);

const char* parse_trade_capture_report(const MessageHeader& header,
                                       const char* p,
                                       TradeCaptureReport* report);

const char* parse_trade_capture_report_ack(const MessageHeader& header,
                                           const char* p,
                                           TradeCaptureReportAck* ack);

const char* parse_business_reject_message(const MessageHeader& header,
                                          const char* p,
                                          BusinessRejectMessage* msg);

const char* parse_quote_status_report(const MessageHeader& header,
                                      const char* p, QuoteStatusReport* report);

}  // namespace ft::bss

#endif  // OCG_BSS_PROTOCOL_PARSER_H_
