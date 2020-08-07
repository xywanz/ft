// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef BSS_TEST_OCG_HANDLER_H_
#define BSS_TEST_OCG_HANDLER_H_

#include "protocol/protocol.h"

using namespace ft::bss;

class OcgHandler {
 public:
  virtual ~OcgHandler() {}

  virtual void on_logon_msg(MessageHeader* header, LogonMessage* msg) {}

  virtual void on_logout_msg(MessageHeader* header, LogoutMessage* msg) {}

  virtual void on_heartbeat_msg(MessageHeader* header, HeartbeatMessage* msg) {}

  virtual void on_test_request(MessageHeader* header, TestRequest* msg) {}

  virtual void on_resend_request(MessageHeader* header, ResendRequest* msg) {}

  virtual void on_reject_msg(MessageHeader* header, RejectMessage* msg) {}

  virtual void on_sequence_reset_msg(MessageHeader* header,
                                     SequenceResetMessage* msg) {}

  virtual void on_new_order_request(MessageHeader* header,
                                    NewOrderRequest* msg) {}

  virtual void on_amend_request(MessageHeader* header, AmendRequest* msg) {}

  virtual void on_cancel_request(MessageHeader* header, CancelRequest* msg) {}

  virtual void on_mass_cancel_request(MessageHeader* header,
                                      MassCancelRequest* msg) {}

  virtual void on_obo_cancel_request(MessageHeader* header,
                                     OboCancelRequest* msg) {}

  virtual void on_obo_mass_cancel_request(MessageHeader* header,
                                          OboMassCancelRequest* msg) {}

  virtual void on_quote_request(MessageHeader* header, QuoteRequest* msg) {}

  virtual void on_quote_cancel_request(MessageHeader* header,
                                       QuoteCancelRequest* msg) {}

  virtual void on_quote_cancel_request(MessageHeader* header,
                                       TradeCaptureReport* msg) {}

  virtual void on_party_entitlement_request(MessageHeader* header,
                                            PartyEntitlementRequest* msg) {}

  virtual void on_invalid_msg() {}
};

#endif  // BSS_TEST_OCG_HANDLER_H_
