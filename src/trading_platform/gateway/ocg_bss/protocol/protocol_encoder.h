// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef OCG_BSS_PROTOCOL_ENCODER_H_
#define OCG_BSS_PROTOCOL_ENCODER_H_

#include <cassert>
#include <cstring>
#include <string>
#include <utility>
#include <vector>

#include "protocol/crc32c.h"
#include "protocol/protocol.h"

namespace ft::bss {

struct MsgBuffer {
  uint32_t size;
  char data[1024];
};

// Ensure the size of MsgBuffer::buffer is larger than that of any request sent
// by bss
inline constexpr const std::size_t kMsgBufSize = sizeof(MsgBuffer::data);
inline constexpr const std::size_t kHeaderTrailerSize =
    sizeof(MessageHeader) + sizeof(MessageTrailer);
static_assert(kMsgBufSize >= kHeaderTrailerSize + sizeof(LogonMessage));
static_assert(kMsgBufSize >= kHeaderTrailerSize + sizeof(LogoutMessage));
static_assert(kMsgBufSize >= kHeaderTrailerSize + sizeof(HeartbeatMessage));
static_assert(kMsgBufSize >= kHeaderTrailerSize + sizeof(TestRequest));
static_assert(kMsgBufSize >= kHeaderTrailerSize + sizeof(ResendRequest));
static_assert(kMsgBufSize >= kHeaderTrailerSize + sizeof(SequenceResetMessage));
static_assert(kMsgBufSize >= kHeaderTrailerSize + sizeof(RejectMessage));
static_assert(kMsgBufSize >=
              kHeaderTrailerSize + sizeof(PartyEntitlementRequest));
static_assert(kMsgBufSize >= kHeaderTrailerSize + sizeof(NewOrderRequest));
static_assert(kMsgBufSize >= kHeaderTrailerSize + sizeof(CancelRequest));
static_assert(kMsgBufSize >= kHeaderTrailerSize + sizeof(AmendRequest));
static_assert(kMsgBufSize >= kHeaderTrailerSize + sizeof(MassCancelRequest));
static_assert(kMsgBufSize >= kHeaderTrailerSize + sizeof(OboCancelRequest));
static_assert(kMsgBufSize >= kHeaderTrailerSize + sizeof(OboMassCancelRequest));
static_assert(kMsgBufSize >= kHeaderTrailerSize + sizeof(TradeCaptureReport));
static_assert(kMsgBufSize >= kHeaderTrailerSize + sizeof(QuoteRequest));
static_assert(kMsgBufSize >= kHeaderTrailerSize + sizeof(QuoteCancelRequest));
static_assert(kMsgBufSize >= kHeaderTrailerSize + sizeof(QuoteRequest));

class BinaryMessageEncoder {
 public:
  BinaryMessageEncoder();

  void encode_msg(const LogonMessage& msg, MsgBuffer* buffer);

  void encode_msg(const LogoutMessage& msg, MsgBuffer* buffer);

  void encode_msg(const SequenceResetMessage& msg, MsgBuffer* buffer);

  void encode_msg(const HeartbeatMessage& msg, MsgBuffer* buffer);

  void encode_msg(const TestRequest& msg, MsgBuffer* buffer);

  void encode_msg(const RejectMessage& msg, MsgBuffer* buffer);

  void encode_msg(const ResendRequest& msg, MsgBuffer* buffer);

  void encode_msg(const NewOrderRequest& req, MsgBuffer* buffer);

  void encode_msg(const AmendRequest& req, MsgBuffer* buffer);

  void encode_msg(const CancelRequest& req, MsgBuffer* buffer);

  void encode_msg(const MassCancelRequest& req, MsgBuffer* buffer);

  void encode_msg(const OboCancelRequest& req, MsgBuffer* buffer);

  void encode_msg(const OboMassCancelRequest& req, MsgBuffer* buffer);

  void encode_msg(const QuoteRequest& req, MsgBuffer* buffer);

  void encode_msg(const QuoteCancelRequest& req, MsgBuffer* buffer);

  void encode_msg(const TradeCaptureReport& report, MsgBuffer* buffer);

  void encode_msg(const PartyEntitlementRequest& req, MsgBuffer* buffer);

  void encode_msg(const LookupRequest& req, MsgBuffer* buffer);

  void set_comp_id(const std::string& comp_id) { comp_id_ = comp_id; }

  void set_next_seq_number(uint32_t next_seq_number) {
    next_seq_number_ = next_seq_number;
  }

  void set_poss_dup_flag(PossDupFlag flag) { poss_dup_flag_ = flag; }

  void set_poss_resend_flag(PossResendFlag flag) { poss_resend_flag_ = flag; }

 protected:
  void encode_new_trade_capture_report(const TradeCaptureReport& report,
                                       MsgBuffer* buffer);

  void encode_cancel_trade_capture_report(const TradeCaptureReport& report,
                                          MsgBuffer* buffer);

  template <class CharArray,
            std::enable_if_t<std::is_array_v<CharArray>, int> = 0>
  void encode(const CharArray& array) {
    strncpy(cur_msg_buf_->data + cur_msg_buf_->size, array, sizeof(CharArray));
    cur_msg_buf_->size += sizeof(CharArray);
  }

  template <class NumericType,
            std::enable_if_t<std::is_arithmetic_v<NumericType>, int> = 0>
  void encode(const NumericType& field) {
    *reinterpret_cast<NumericType*>(cur_msg_buf_->data + cur_msg_buf_->size) =
        field;
    cur_msg_buf_->size += sizeof(NumericType);
  }

  template <std::size_t N>
  void encode(const AlphanumericVariableLength<N>& field) {
    encode(field.len);
    strncpy(cur_msg_buf_->data + cur_msg_buf_->size, field.data, N);
    cur_msg_buf_->size += field.len;
  }

  MessageHeader* encode_header(MessageType type) {
    auto header = reinterpret_cast<MessageHeader*>(cur_msg_buf_->data);
    header->start_of_message = 0x02;
    header->message_type = type;
    header->sequence_number = next_seq_number_++;
    header->poss_dup_flag = poss_dup_flag_;
    header->poss_resend_flag = poss_resend_flag_;
    strncpy(header->comp_id, comp_id_.c_str(), sizeof(CompId));
    memset(header->body_fields_presence_map, 0, sizeof(PresenceBitmap));

    cur_msg_buf_->size += sizeof(MessageHeader);
    return header;
  }

  void fill_header_and_trailer() {
    auto header = reinterpret_cast<MessageHeader*>(cur_msg_buf_->data);
    uint32_t head_body_len = cur_msg_buf_->size;
    header->length = head_body_len + sizeof(Checksum);
    encode(crc32c(0xffffffffUL, reinterpret_cast<uint8_t*>(header),
                  head_body_len));
    cur_msg_buf_ = nullptr;
  }

 protected:
  std::string comp_id_;
  uint32_t next_seq_number_ = 0;
  PossDupFlag poss_dup_flag_ = 0;
  PossResendFlag poss_resend_flag_ = 0;

  MsgBuffer* cur_msg_buf_ = nullptr;
};

inline void set_presence(PresenceBitmap bitmap, uint32_t i) {
  assert(i < sizeof(PresenceBitmap) * 8);
  bitmap[i >> 3] |= (0b10000000U >> (i & 0x7U));
}

template <class Message>
inline OcgMessageType get_msg_type();

template <>
inline OcgMessageType get_msg_type<LogonMessage>() {
  return LOGON;
}

template <>
inline OcgMessageType get_msg_type<LogoutMessage>() {
  return LOGOUT;
}

template <>
inline OcgMessageType get_msg_type<HeartbeatMessage>() {
  return HEARTBEAT;
}

template <>
inline OcgMessageType get_msg_type<TestRequest>() {
  return TEST_REQUEST;
}

template <>
inline OcgMessageType get_msg_type<ResendRequest>() {
  return RESEND_REQUEST;
}

template <>
inline OcgMessageType get_msg_type<SequenceResetMessage>() {
  return SEQUENCE_RESET;
}

template <>
inline OcgMessageType get_msg_type<RejectMessage>() {
  return REJECT;
}

template <>
inline OcgMessageType get_msg_type<NewOrderRequest>() {
  return NEW_ORDER;
}

template <>
inline OcgMessageType get_msg_type<AmendRequest>() {
  return AMEND_REQUEST;
}

template <>
inline OcgMessageType get_msg_type<CancelRequest>() {
  return CANCEL_REQUEST;
}

template <>
inline OcgMessageType get_msg_type<MassCancelRequest>() {
  return MASS_CANCEL_REQUEST;
}

template <>
inline OcgMessageType get_msg_type<OboCancelRequest>() {
  return OBO_CANCEL_REQUEST;
}

template <>
inline OcgMessageType get_msg_type<OboMassCancelRequest>() {
  return OBO_MASS_CANCEL_REQUEST;
}

template <>
inline OcgMessageType get_msg_type<QuoteRequest>() {
  return QUOTE;
}

template <>
inline OcgMessageType get_msg_type<QuoteCancelRequest>() {
  return QUOTE_CANCEL;
}

template <>
inline OcgMessageType get_msg_type<TradeCaptureReport>() {
  return TRADE_CAPTURE_REPORT;
}

template <>
inline OcgMessageType get_msg_type<PartyEntitlementRequest>() {
  return PARTY_ENTITLEMENTS_REQUEST;
}

template <>
inline OcgMessageType get_msg_type<ExecutionReport>() {
  return EXECUTION_REPORT;
}

template <>
inline OcgMessageType get_msg_type<OrderMassCancelReport>() {
  return ORDER_MASS_CANCEL_REPORT;
}

template <>
inline OcgMessageType get_msg_type<QuoteStatusReport>() {
  return QUOTE_STATUS_REPORT;
}

template <>
inline OcgMessageType get_msg_type<TradeCaptureReportAck>() {
  return TRADE_CAPTURE_REPORT_ACK;
}

template <>
inline OcgMessageType get_msg_type<BusinessRejectMessage>() {
  return BUSINESS_MESSAGE_REJECT;
}

}  // namespace ft::bss

#endif  // OCG_BSS_PROTOCOL_ENCODER_H_
