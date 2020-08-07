// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef BSS_TEST_OCG_ENCODER_H_
#define BSS_TEST_OCG_ENCODER_H_

#include "protocol/protocol_encoder.h"

using namespace ft::bss;

class OcgEncoder : public BinaryMessageEncoder {
 public:
  void encode_msg(const ExecutionReport& report, MsgBuffer* buffer);

  void encode_msg(const TradeCaptureReportAck& ack, MsgBuffer* buffer);

  void encode_msg(const QuoteStatusReport& report, MsgBuffer* buffer);

  void encode_msg(const OrderMassCancelReport& report, MsgBuffer* buffer);

  void encode_msg(const BusinessRejectMessage& msg, MsgBuffer* buffer);

  void encode_msg(const LookupResponse& rsp, MsgBuffer* buffer);

  using BinaryMessageEncoder::encode_msg;
};

#endif  // BSS_TEST_OCG_ENCODER_H_
