// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "protocol/protocol_encoder.h"

namespace ft::bss {

BinaryMessageEncoder::BinaryMessageEncoder() {}

void BinaryMessageEncoder::encode_msg(const LogonMessage& msg,
                                      MsgBuffer* buffer) {
  buffer->size = 0;
  cur_msg_buf_ = buffer;
  auto header = encode_header(LOGON);
  header->body_fields_presence_map[0] = 0b10100000;

  encode(msg.password);
  if (msg.new_password[0] != 0) {
    set_presence(header->body_fields_presence_map, 1);
    encode(msg.new_password);
  }
  encode(msg.next_expected_message_sequence);

  fill_header_and_trailer();
}

void BinaryMessageEncoder::encode_msg(const LogoutMessage& msg,
                                      MsgBuffer* buffer) {
  buffer->size = 0;
  cur_msg_buf_ = buffer;
  auto header = encode_header(LOGOUT);

  if (msg.logout_text.len > 0) {
    set_presence(header->body_fields_presence_map, 0);
    encode(msg.logout_text);
  }
  set_presence(header->body_fields_presence_map, 1);  // take care
  encode(msg.session_status);

  fill_header_and_trailer();
}

void BinaryMessageEncoder::encode_msg(const SequenceResetMessage& msg,
                                      MsgBuffer* buffer) {
  buffer->size = 0;
  cur_msg_buf_ = buffer;
  auto header = encode_header(SEQUENCE_RESET);
  header->body_fields_presence_map[0] = 0b11000000;

  GapFill gap_fill = msg.gap_fill;
  if (msg.gap_fill != 'Y') gap_fill = 'N';
  encode(gap_fill);
  encode(msg.new_sequence_number);

  fill_header_and_trailer();
}

void BinaryMessageEncoder::encode_msg(const HeartbeatMessage& msg,
                                      MsgBuffer* buffer) {
  buffer->size = 0;
  cur_msg_buf_ = buffer;
  auto header = encode_header(HEARTBEAT);

  if (msg.reference_test_request_id != 0) {
    set_presence(header->body_fields_presence_map, 0);
    encode(msg.reference_test_request_id);
  }

  fill_header_and_trailer();
}

void BinaryMessageEncoder::encode_msg(const TestRequest& msg,
                                      MsgBuffer* buffer) {
  buffer->size = 0;
  cur_msg_buf_ = buffer;
  auto header = encode_header(TEST_REQUEST);
  header->body_fields_presence_map[0] = 0b10000000;

  encode(msg.test_request_id);
  fill_header_and_trailer();
}

void BinaryMessageEncoder::encode_msg(const RejectMessage& msg,
                                      MsgBuffer* buffer) {
  buffer->size = 0;
  cur_msg_buf_ = buffer;
  auto header = encode_header(REJECT);
  header->body_fields_presence_map[0] = 0b10101000;

  encode(msg.message_reject_code);
  if (msg.reason.len > 0) {
    set_presence(header->body_fields_presence_map, 1);
    encode(msg.reason);
  }
  encode(msg.reference_message_type);  // N, but Y here
  if (msg.reference_field_name[0] != 0) {
    set_presence(header->body_fields_presence_map, 3);
    encode(msg.reference_field_name);
  }
  encode(msg.reference_sequence_number);
  if (msg.client_order_id[0] != 0) {
    set_presence(header->body_fields_presence_map, 5);
    encode(msg.client_order_id);
  }

  fill_header_and_trailer();
}

void BinaryMessageEncoder::encode_msg(const ResendRequest& msg,
                                      MsgBuffer* buffer) {
  buffer->size = 0;
  cur_msg_buf_ = buffer;
  auto header = encode_header(RESEND_REQUEST);
  header->body_fields_presence_map[0] = 0b11000000;

  encode(msg.start_sequence);
  encode(msg.end_sequence);

  fill_header_and_trailer();
}

void BinaryMessageEncoder::encode_msg(const NewOrderRequest& req,
                                      MsgBuffer* buffer) {
  buffer->size = 0;
  cur_msg_buf_ = buffer;
  auto header = encode_header(NEW_ORDER);
  header->body_fields_presence_map[0] = 0b11110011;
  header->body_fields_presence_map[1] = 0b10110000;
  header->body_fields_presence_map[2] = 0b00100000;

  encode(req.client_order_id);
  encode(req.submitting_broker_id);
  encode(req.security_id);
  encode(req.security_id_source);
  if (req.security_exchange[0] != 0) {
    set_presence(header->body_fields_presence_map, 4);
    encode(req.security_exchange);
  }
  if (req.broker_location_id[0] != 0) {
    set_presence(header->body_fields_presence_map, 5);
    encode(req.broker_location_id);
  }
  encode(req.transaction_time);
  encode(req.side);
  encode(req.order_type);
  if (req.order_type == ORDER_TYPE_LIMIT) {
    set_presence(header->body_fields_presence_map, 9);
    encode(req.price);
  }
  encode(req.order_quantity);
  encode(req.tif);
  if (req.position_effect != 0) {
    set_presence(header->body_fields_presence_map, 12);
    encode(req.position_effect);
  }
  if (req.lot_type == LOT_TYPE_BOARD) {
    if (req.order_restrictions[0] != 0) {
      set_presence(header->body_fields_presence_map, 13);
      encode(req.order_restrictions);
    }
    if (req.max_price_levels != 0) {
      set_presence(header->body_fields_presence_map, 14);
      encode(req.max_price_levels);
    }
  }
  if (req.order_capacity != 0) {
    set_presence(header->body_fields_presence_map, 15);
    encode(req.order_capacity);
  }
  if (req.text.len != 0) {
    set_presence(header->body_fields_presence_map, 16);
    encode(req.text);
  }
  if (req.execution_instructions[0] != 0) {
    set_presence(header->body_fields_presence_map, 17);
    encode(req.execution_instructions);
  }
  encode(req.disclosure_instructions);
  if (req.lot_type == LOT_TYPE_ODD_SPEC) {
    set_presence(header->body_fields_presence_map, 19);
    encode(req.lot_type);
  }

  fill_header_and_trailer();
}

void BinaryMessageEncoder::encode_msg(const AmendRequest& req,
                                      MsgBuffer* buffer) {
  buffer->size = 0;
  cur_msg_buf_ = buffer;
  auto header = encode_header(AMEND_REQUEST);
  header->body_fields_presence_map[0] = 0b11110011;
  header->body_fields_presence_map[1] = 0b10101100;
  header->body_fields_presence_map[2] = 0b00001000;

  encode(req.client_order_id);
  encode(req.submitting_broker_id);
  encode(req.security_id);
  encode(req.security_id_source);
  if (req.security_exchange[0] != 0) {
    set_presence(header->body_fields_presence_map, 4);
    encode(req.security_exchange);
  }
  if (req.broker_location_id[0] != 0) {
    set_presence(header->body_fields_presence_map, 5);
    encode(req.broker_location_id);
  }
  encode(req.transaction_time);
  encode(req.side);
  encode(req.original_client_order_id);
  if (req.order_id[0] != 0) {
    set_presence(header->body_fields_presence_map, 9);
    encode(req.order_id);
  }
  encode(req.order_type);
  if (req.order_type == ORDER_TYPE_LIMIT) {
    set_presence(header->body_fields_presence_map, 11);
    encode(req.price);
  }
  encode(req.order_quantity);
  encode(req.tif);
  if (req.position_effect != 0) {
    set_presence(header->body_fields_presence_map, 14);
    encode(req.position_effect);
  }
  if (req.order_restrictions[0] != 0) {
    set_presence(header->body_fields_presence_map, 15);
    encode(req.order_restrictions);
  }
  if (req.max_price_levels != 0) {
    set_presence(header->body_fields_presence_map, 16);
    encode(req.max_price_levels);
  }
  if (req.order_capacity != 0) {
    set_presence(header->body_fields_presence_map, 17);
    encode(req.order_capacity);
  }
  if (req.text.len != 0) {
    set_presence(header->body_fields_presence_map, 18);
    encode(req.text);
  }
  if (req.execution_instructions[0] != 0) {
    set_presence(header->body_fields_presence_map, 19);
    encode(req.execution_instructions);
  }
  encode(req.disclosure_instructions);

  fill_header_and_trailer();
}

void BinaryMessageEncoder::encode_msg(const CancelRequest& req,
                                      MsgBuffer* buffer) {
  buffer->size = 0;
  cur_msg_buf_ = buffer;
  auto header = encode_header(CANCEL_REQUEST);
  header->body_fields_presence_map[0] = 0b11110011;
  header->body_fields_presence_map[1] = 0b10000000;

  encode(req.client_order_id);
  encode(req.submitting_broker_id);
  encode(req.security_id);
  encode(req.security_id_source);
  if (req.security_exchange[0] != 0) {
    set_presence(header->body_fields_presence_map, 4);
    encode(req.security_exchange);
  }
  if (req.broker_location_id[0] != 0) {
    set_presence(header->body_fields_presence_map, 5);
    encode(req.broker_location_id);
  }
  encode(req.transaction_time);
  encode(req.side);
  encode(req.original_client_order_id);
  if (req.order_id[0] != 0) {
    set_presence(header->body_fields_presence_map, 9);
    encode(req.order_id);
  }
  if (req.text.len != 0) {
    set_presence(header->body_fields_presence_map, 10);
    encode(req.text);
  }

  fill_header_and_trailer();
}

void BinaryMessageEncoder::encode_msg(const MassCancelRequest& req,
                                      MsgBuffer* buffer) {
  buffer->size = 0;
  cur_msg_buf_ = buffer;
  auto header = encode_header(MASS_CANCEL_REQUEST);
  header->body_fields_presence_map[0] = 0b11000010;
  header->body_fields_presence_map[1] = 0b10000000;

  encode(req.client_order_id);
  encode(req.submitting_broker_id);
  if (req.security_id[0] != 0) {
    set_presence(header->body_fields_presence_map, 2);
    encode(req.security_id);
  }
  if (req.security_id_source != 0) {
    set_presence(header->body_fields_presence_map, 3);
    encode(req.security_id_source);
  }
  if (req.security_exchange[0] != 0) {
    set_presence(header->body_fields_presence_map, 4);
    encode(req.security_exchange);
  }
  if (req.broker_location_id[0] != 0) {
    set_presence(header->body_fields_presence_map, 5);
    encode(req.broker_location_id);
  }
  encode(req.transaction_time);
  if (req.side != 0) {
    set_presence(header->body_fields_presence_map, 7);
    encode(req.side);
  }
  encode(req.mass_cancel_request_type);
  if (req.market_segment_id[0] != 0) {
    set_presence(header->body_fields_presence_map, 9);
    encode(req.market_segment_id);
  }

  fill_header_and_trailer();
}

void BinaryMessageEncoder::encode_msg(const OboCancelRequest& req,
                                      MsgBuffer* buffer) {
  buffer->size = 0;
  cur_msg_buf_ = buffer;
  auto header = encode_header(OBO_CANCEL_REQUEST);
  header->body_fields_presence_map[0] = 0b11110011;
  header->body_fields_presence_map[1] = 0b01100000;

  encode(req.client_order_id);
  encode(req.submitting_broker_id);
  encode(req.security_id);
  encode(req.security_id_source);
  if (req.security_exchange[0] != 0) {
    set_presence(header->body_fields_presence_map, 4);
    encode(req.security_exchange);
  }
  if (req.broker_location_id[0] != 0) {
    set_presence(header->body_fields_presence_map, 5);
    encode(req.broker_location_id);
  }
  encode(req.transaction_time);
  encode(req.side);
  if (req.original_client_order_id[0] != 0) {
    set_presence(header->body_fields_presence_map, 8);
    encode(req.original_client_order_id);
  }
  encode(req.order_id);
  encode(req.owning_broker_id);
  if (req.text.len != 0) {
    set_presence(header->body_fields_presence_map, 11);
    encode(req.text);
  }

  fill_header_and_trailer();
}

void BinaryMessageEncoder::encode_msg(const OboMassCancelRequest& req,
                                      MsgBuffer* buffer) {
  buffer->size = 0;
  cur_msg_buf_ = buffer;
  auto header = encode_header(OBO_MASS_CANCEL_REQUEST);
  header->body_fields_presence_map[0] = 0b11000010;
  header->body_fields_presence_map[1] = 0b10100000;

  encode(req.client_order_id);
  encode(req.submitting_broker_id);
  if (req.security_id[0] != 0) {
    set_presence(header->body_fields_presence_map, 2);
    encode(req.security_id);
  }
  if (req.security_id_source != 0) {
    set_presence(header->body_fields_presence_map, 3);
    encode(req.security_id_source);
  }
  if (req.security_exchange[0] != 0) {
    set_presence(header->body_fields_presence_map, 4);
    encode(req.security_exchange);
  }
  if (req.broker_location_id[0] != 0) {
    set_presence(header->body_fields_presence_map, 5);
    encode(req.broker_location_id);
  }
  encode(req.transaction_time);
  if (req.side != 0) {
    set_presence(header->body_fields_presence_map, 7);
    encode(req.side);
  }
  encode(req.mass_cancel_request_type);
  if (req.market_segment_id[0] != 0) {
    set_presence(header->body_fields_presence_map, 9);
    encode(req.market_segment_id);
  }
  encode(req.owning_broker_id);

  fill_header_and_trailer();
}

void BinaryMessageEncoder::encode_msg(const QuoteRequest& req,
                                      MsgBuffer* buffer) {
  buffer->size = 0;
  cur_msg_buf_ = buffer;
  auto header = encode_header(QUOTE);
  header->body_fields_presence_map[0] = 0b10110110;
  header->body_fields_presence_map[1] = 0b01111100;

  encode(req.submitting_broker_id);
  if (req.broker_location_id[0] != 0) {
    set_presence(header->body_fields_presence_map, 1);
    encode(req.broker_location_id);
  }
  encode(req.security_id);
  encode(req.security_id_source);
  if (req.security_exchange[0] != 0) {
    set_presence(header->body_fields_presence_map, 4);
    encode(req.security_exchange);
  }
  encode(req.quote_bid_id);
  encode(req.quote_offer_id);
  if (req.quote_type != 0) {
    set_presence(header->body_fields_presence_map, 7);
    encode(req.quote_type);
  }
  if (req.side != 0) {
    set_presence(header->body_fields_presence_map, 8);
    encode(req.side);
  }
  encode(req.bid_size);
  encode(req.offer_size);
  encode(req.bid_price);
  encode(req.offer_price);
  encode(req.transaction_time);
  if (req.position_effect != 0) {
    set_presence(header->body_fields_presence_map, 14);
    encode(req.position_effect);
  }
  if (req.order_restrictions[0] != 0) {
    set_presence(header->body_fields_presence_map, 15);
    encode(req.order_restrictions);
  }
  if (req.text.len != 0) {
    set_presence(header->body_fields_presence_map, 16);
    encode(req.text);
  }
  if (req.execution_instructions[0] != 0) {
    set_presence(header->body_fields_presence_map, 17);
    encode(req.execution_instructions);
  }

  fill_header_and_trailer();
}

void BinaryMessageEncoder::encode_msg(const QuoteCancelRequest& req,
                                      MsgBuffer* buffer) {
  buffer->size = 0;
  cur_msg_buf_ = buffer;
  auto header = encode_header(QUOTE_CANCEL);
  header->body_fields_presence_map[0] = 0b10000110;

  encode(req.submitting_broker_id);
  if (req.broker_location_id[0] != 0) {
    set_presence(header->body_fields_presence_map, 1);
    encode(req.broker_location_id);
  }
  if (req.security_id[0] != 0) {
    set_presence(header->body_fields_presence_map, 2);
    encode(req.security_id);
  }
  if (req.security_id_source != 0) {
    set_presence(header->body_fields_presence_map, 3);
    encode(req.security_id_source);
  }
  if (req.security_exchange[0] != 0) {
    set_presence(header->body_fields_presence_map, 4);
    encode(req.security_exchange);
  }
  encode(req.quote_message_id);
  encode(req.quote_cancel_type);

  fill_header_and_trailer();
}

void BinaryMessageEncoder::encode_msg(const TradeCaptureReport& report,
                                      MsgBuffer* buffer) {
  buffer->size = 0;
  cur_msg_buf_ = buffer;
  if (report.trade_report_trans_type == 0) {
    encode_new_trade_capture_report(report, buffer);
  } else if (report.trade_report_trans_type == 5) {
    encode_cancel_trade_capture_report(report, buffer);
  } else {
    cur_msg_buf_ = nullptr;
  }
}

void BinaryMessageEncoder::encode_new_trade_capture_report(
    const TradeCaptureReport& report, MsgBuffer* buffer) {
  auto header = encode_header(TRADE_CAPTURE_REPORT);
  header->body_fields_presence_map[0] = 0b11101001;
  header->body_fields_presence_map[1] = 0b10110111;
  header->body_fields_presence_map[2] = 0b10000000;

  encode(report.trade_report_id);
  encode(report.trade_report_trans_type);  //
  encode(report.trade_report_type);
  if (report.trade_handling_instructions != 0) {
    set_presence(header->body_fields_presence_map, 3);
    encode(report.trade_handling_instructions);
  }
  encode(report.submitting_broker_id);
  if (report.counterparty_broker_id[0] != 0) {
    set_presence(header->body_fields_presence_map, 5);
    encode(report.counterparty_broker_id);
  }
  if (report.broker_location_id[0] != 0) {
    set_presence(header->body_fields_presence_map, 6);
    encode(report.broker_location_id);
  }
  encode(report.security_id);
  encode(report.security_id_source);
  if (report.security_exchange[0] != 0) {
    set_presence(header->body_fields_presence_map, 9);
    encode(report.security_exchange);
  }
  encode(report.side);
  encode(report.transaction_time);
  encode(report.trade_type);
  encode(report.execution_quantity);
  encode(report.execution_price);
  encode(report.clearing_instruction);  //
  if (report.position_effect != 0) {
    set_presence(header->body_fields_presence_map, 17);
    encode(report.position_effect);
  }
  if (report.order_capacity != 0) {
    set_presence(header->body_fields_presence_map, 19);
    encode(report.order_capacity);
  }
  if (report.order_category != 0) {
    set_presence(header->body_fields_presence_map, 20);
    encode(report.order_category);
  }
  if (report.text.len != 0) {
    set_presence(header->body_fields_presence_map, 21);
    encode(report.text);
  }
  if (report.execution_instructions[0] != 0) {
    set_presence(header->body_fields_presence_map, 22);
    encode(report.execution_instructions);
  }
  if (report.order_id[0] != 0) {
    set_presence(header->body_fields_presence_map, 27);
    encode(report.order_id);
  }

  fill_header_and_trailer();
}

void BinaryMessageEncoder::encode_cancel_trade_capture_report(
    const TradeCaptureReport& report, MsgBuffer* buffer) {
  auto header = encode_header(TRADE_CAPTURE_REPORT);
  header->body_fields_presence_map[0] = 0b11101001;
  header->body_fields_presence_map[1] = 0b10111011;

  encode(report.trade_report_id);
  encode(report.trade_report_trans_type);
  encode(report.trade_report_type);
  if (report.trade_handling_instructions != 0) {
    set_presence(header->body_fields_presence_map, 3);
    encode(report.trade_handling_instructions);
  }
  encode(report.submitting_broker_id);
  if (report.broker_location_id[0] != 0) {
    set_presence(header->body_fields_presence_map, 6);
    encode(report.broker_location_id);
  }
  encode(report.security_id);
  encode(report.security_id_source);
  if (report.security_exchange[0] != 0) {
    set_presence(header->body_fields_presence_map, 9);
    encode(report.security_exchange);
  }
  encode(report.side);
  encode(report.transaction_time);
  encode(report.trade_id);
  encode(report.execution_quantity);
  encode(report.execution_price);

  fill_header_and_trailer();
}

void BinaryMessageEncoder::encode_msg(const PartyEntitlementRequest& req,
                                      MsgBuffer* buffer) {
  buffer->size = 0;
  cur_msg_buf_ = buffer;
  auto header = encode_header(PARTY_ENTITLEMENTS_REQUEST);
  header->body_fields_presence_map[0] = 0b10000000;

  encode(req.entitlement_request_id);

  fill_header_and_trailer();
}

void BinaryMessageEncoder::encode_msg(const LookupRequest& req,
                                      MsgBuffer* buffer) {
  buffer->size = 0;
  cur_msg_buf_ = buffer;
  auto header = encode_header(LOOKUP_REQUEST);
  header->sequence_number = 1;  // LookupRequest的SeqNum被规定为1
  header->body_fields_presence_map[0] = 0b11000000;

  encode(req.type_of_service);
  encode(req.protocol_type);

  fill_header_and_trailer();
}

}  // namespace ft::bss
