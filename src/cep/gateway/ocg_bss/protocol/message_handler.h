// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef OCG_BSS_MESSAGE_HANDLER_H_
#define OCG_BSS_MESSAGE_HANDLER_H_

#include "protocol/protocol.h"

namespace ft::bss {

class MessageHandler {
 public:
  virtual ~MessageHandler() {}

  virtual void on_logon_msg(MessageHeader* header, LogonMessage* msg) {}

  virtual void on_logout_msg(MessageHeader* header, LogoutMessage* msg) {}

  virtual void on_heartbeat_msg(MessageHeader* header, HeartbeatMessage* msg) {}

  virtual void on_test_request(MessageHeader* header, TestRequest* msg) {}

  virtual void on_resend_request(MessageHeader* header, ResendRequest* msg) {}

  virtual void on_reject_msg(MessageHeader* header, RejectMessage* msg) {}

  virtual void on_sequence_reset_msg(MessageHeader* header,
                                     SequenceResetMessage* msg) {}

  virtual void on_business_reject_msg(MessageHeader* header,
                                      BusinessRejectMessage* msg) {}

  virtual void on_execution_report(MessageHeader* header,
                                   ExecutionReport* msg) {}

  virtual void on_mass_cancel_report(MessageHeader* header,
                                     OrderMassCancelReport* msg) {}

  virtual void on_quote_status_report(MessageHeader* header,
                                      QuoteStatusReport* msg) {}

  virtual void on_trade_capture_report(MessageHeader* header,
                                       TradeCaptureReport* msg) {}

  virtual void on_trade_capture_report_ack(MessageHeader* header,
                                           TradeCaptureReportAck* msg) {}

  virtual void on_invalid_msg() {}
};

}  // namespace ft::bss

#endif  // OCG_BSS_MESSAGE_HANDLER_H
