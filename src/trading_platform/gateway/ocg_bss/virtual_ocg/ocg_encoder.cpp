// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "virtual_ocg/ocg_encoder.h"

using namespace ft::bss;

void OcgEncoder::encode_msg(const ExecutionReport& report, MsgBuffer* buffer) {
  buffer->size = 0;
  cur_msg_buf_ = buffer;
  auto header = encode_header(EXECUTION_REPORT);
  header->body_fields_presence_map[0] = 0b11110011;
  header->body_fields_presence_map[1] = 0b01000000;
  header->body_fields_presence_map[2] = 0b00000111;
  header->body_fields_presence_map[3] = 0b11000000;

  encode(report.client_order_id);
  encode(report.submitting_broker_id);
  encode(report.security_id);
  encode(report.security_id_source);
  if (report.security_exchange[0] != 0) {
    set_presence(header->body_fields_presence_map, 4);
    encode(report.security_exchange);
  }
  if (report.broker_location_id[0] != 0) {
    set_presence(header->body_fields_presence_map, 5);
    encode(report.broker_location_id);
  }
  encode(report.transaction_time);
  encode(report.side);
  if (report.original_client_order_id[0] != 0) {
    set_presence(header->body_fields_presence_map, 8);
    encode(report.original_client_order_id);
  }
  encode(report.order_id);
  if (report.owning_broker_id[0] != 0) {
    set_presence(header->body_fields_presence_map, 10);
    encode(report.owning_broker_id);
  }
  if (report.order_type != 0) {
    set_presence(header->body_fields_presence_map, 11);
    encode(report.order_type);
  }
  if (report.price != 0) {
    set_presence(header->body_fields_presence_map, 12);
    encode(report.price);
  }
  if (report.order_quantity != 0) {
    set_presence(header->body_fields_presence_map, 13);
    encode(report.order_quantity);
  }
  if (1) {
    set_presence(header->body_fields_presence_map, 14);
    encode(report.tif);
  }
  if (report.position_effect != 0) {
    set_presence(header->body_fields_presence_map, 15);
    encode(report.position_effect);
  }
  if (report.order_restrictions[0] != 0) {
    set_presence(header->body_fields_presence_map, 16);
    encode(report.order_restrictions);
  }
  if (report.max_price_levels != 0) {
    set_presence(header->body_fields_presence_map, 17);
    encode(report.max_price_levels);
  }
  if (report.order_capacity != 0) {
    set_presence(header->body_fields_presence_map, 18);
    encode(report.order_capacity);
  }
  if (report.text.len > 0) {
    set_presence(header->body_fields_presence_map, 19);
    encode(report.text);
  }
  if (report.reason.len > 0) {
    set_presence(header->body_fields_presence_map, 20);
    encode(report.reason);
  }
  encode(report.execution_id);
  encode(report.order_status);
  encode(report.exec_type);
  encode(report.cumulative_quantity);
  encode(report.leaves_quantity);
  if (report.order_reject_code != 0) {
    set_presence(header->body_fields_presence_map, 26);
    encode(report.order_reject_code);
  }
  if (report.lot_type != 0) {
    set_presence(header->body_fields_presence_map, 27);
    encode(report.lot_type);
  }
  if (report.exec_restatement_reason != 0) {
    set_presence(header->body_fields_presence_map, 28);
    encode(report.exec_restatement_reason);
  }
  if (report.cancel_reject_code != 0) {
    set_presence(header->body_fields_presence_map, 29);
    encode(report.cancel_reject_code);
  }
  if (report.match_type != 0) {
    set_presence(header->body_fields_presence_map, 30);
    encode(report.match_type);
  }
  if (report.counterparty_broker_id[0] != 0) {
    set_presence(header->body_fields_presence_map, 31);
    encode(report.counterparty_broker_id);
  }
  if (report.execution_quantity != 0) {
    set_presence(header->body_fields_presence_map, 32);
    encode(report.execution_quantity);
  }
  if (report.execution_price != 0) {
    set_presence(header->body_fields_presence_map, 33);
    encode(report.execution_price);
  }
  if (report.reference_execution_id[0] != 0) {
    set_presence(header->body_fields_presence_map, 34);
    encode(report.reference_execution_id);
  }
  if (report.order_category != 0) {
    set_presence(header->body_fields_presence_map, 35);
    encode(report.order_category);
  }
  if (report.amend_reject_code != 0) {
    set_presence(header->body_fields_presence_map, 36);
    encode(report.amend_reject_code);
  }
  // 37 skip
  if (report.trade_match_id[0] != 0) {
    set_presence(header->body_fields_presence_map, 38);
    encode(report.trade_match_id);
  }
  if (report.exchange_trade_type != 0) {
    set_presence(header->body_fields_presence_map, 39);
    encode(report.exchange_trade_type);
  }

  fill_header_and_trailer();
}

void OcgEncoder::encode_msg(const TradeCaptureReportAck& ack,
                            MsgBuffer* buffer) {
  buffer->size = 0;
  cur_msg_buf_ = buffer;
  auto header = encode_header(EXECUTION_REPORT);
  header->body_fields_presence_map[0] = 0b11101001;
  header->body_fields_presence_map[1] = 0b10110000;

  encode(ack.trade_report_id);
  encode(ack.trade_report_trans_type);  //
  encode(ack.trade_report_type);
  if (ack.trade_handling_instructions != 0) {
    set_presence(header->body_fields_presence_map, 3);
    encode(ack.trade_handling_instructions);
  }
  encode(ack.submitting_broker_id);
  if (ack.counterparty_broker_id[0] != 0) {
    set_presence(header->body_fields_presence_map, 5);
    encode(ack.counterparty_broker_id);
  }
  if (ack.broker_location_id[0] != 0) {
    set_presence(header->body_fields_presence_map, 6);
    encode(ack.broker_location_id);
  }
  encode(ack.security_id);
  encode(ack.security_id_source);
  if (ack.security_exchange[0] != 0) {
    set_presence(header->body_fields_presence_map, 9);
    encode(ack.security_exchange);
  }
  encode(ack.side);
  encode(ack.transaction_time);
  if (ack.trade_id[0] != 0) {
    set_presence(header->body_fields_presence_map, 12);
    encode(ack.trade_id);
  }
  if (ack.trade_report_status != 0) {
    set_presence(header->body_fields_presence_map, 13);
    encode(ack.trade_report_status);
  }
  if (ack.trade_report_reject_code != 0) {
    set_presence(header->body_fields_presence_map, 14);
    encode(ack.trade_report_reject_code);
  }
  if (ack.reason.len > 0) {
    set_presence(header->body_fields_presence_map, 15);
    encode(ack.reason);
  }

  fill_header_and_trailer();
}

void OcgEncoder::encode_msg(const QuoteStatusReport& report,
                            MsgBuffer* buffer) {
  buffer->size = 0;
  cur_msg_buf_ = buffer;
  auto header = encode_header(QUOTE_STATUS_REPORT);
  header->body_fields_presence_map[0] = 0b10000000;
  header->body_fields_presence_map[1] = 0b10000000;

  encode(report.submitting_broker_id);
  if (report.broker_location_id[0] != 0) {
    set_presence(header->body_fields_presence_map, 1);
    encode(report.broker_location_id);
  }
  if (report.security_id[0] != 0) {
    set_presence(header->body_fields_presence_map, 2);
    encode(report.security_id);
  }
  if (report.security_id_source != 0) {
    set_presence(header->body_fields_presence_map, 3);
    encode(report.security_id_source);
  }
  if (report.security_exchange[0] != 0) {
    set_presence(header->body_fields_presence_map, 4);
    encode(report.security_exchange);
  }
  if (report.quote_bid_id[0] != 0) {
    set_presence(header->body_fields_presence_map, 5);
    encode(report.quote_bid_id);
  }
  if (report.quote_offer_id[0] != 0) {
    set_presence(header->body_fields_presence_map, 6);
    encode(report.quote_offer_id);
  }
  if (report.quote_type != 0) {
    set_presence(header->body_fields_presence_map, 7);
    encode(report.quote_type);
  }
  encode(report.transaction_time);
  if (report.quote_message_id[0] != 0) {
    set_presence(header->body_fields_presence_map, 9);
    encode(report.quote_message_id);
  }
  if (report.quote_cancel_type != 0) {
    set_presence(header->body_fields_presence_map, 10);
    encode(report.quote_cancel_type);
  }
  if (report.quote_status != 0) {
    set_presence(header->body_fields_presence_map, 11);
    encode(report.quote_status);
  }
  if (report.quote_reject_code != 0) {
    set_presence(header->body_fields_presence_map, 12);
    encode(report.quote_reject_code);
  }
  if (report.reason.len > 0) {
    set_presence(header->body_fields_presence_map, 13);
    encode(report.reason);
  }

  fill_header_and_trailer();
}

void OcgEncoder::encode_msg(const OrderMassCancelReport& report,
                            MsgBuffer* buffer) {
  buffer->size = 0;
  cur_msg_buf_ = buffer;
  auto header = encode_header(ORDER_MASS_CANCEL_REPORT);
  header->body_fields_presence_map[0] = 0b00000011;
  header->body_fields_presence_map[1] = 0b01100000;

  if (report.client_order_id[0] != 0) {
    set_presence(header->body_fields_presence_map, 0);
    encode(report.client_order_id);
  }
  if (report.submitting_broker_id[0] != 0) {
    set_presence(header->body_fields_presence_map, 1);
    encode(report.submitting_broker_id);
  }
  if (report.security_id[0] != 0) {
    set_presence(header->body_fields_presence_map, 2);
    encode(report.security_id);
  }
  if (report.security_id_source != 0) {
    set_presence(header->body_fields_presence_map, 4);
    encode(report.security_id_source);
  }
  if (report.security_exchange[0] != 0) {
    set_presence(header->body_fields_presence_map, 5);
    encode(report.security_exchange);
  }
  encode(report.transaction_time);
  encode(report.mass_cancel_request_type);
  if (report.owning_broker_id[0] != 0) {
    set_presence(header->body_fields_presence_map, 8);
    encode(report.owning_broker_id);
  }
  encode(report.mass_action_report_id);
  encode(report.mass_cancel_response);
  if (report.mass_cancel_reject_code != 0) {
    set_presence(header->body_fields_presence_map, 11);
    encode(report.mass_cancel_reject_code);
  }
  if (report.reason.len > 0) {
    set_presence(header->body_fields_presence_map, 12);
    encode(report.reason);
  }

  fill_header_and_trailer();
}

void OcgEncoder::encode_msg(const BusinessRejectMessage& msg,
                            MsgBuffer* buffer) {
  buffer->size = 0;
  cur_msg_buf_ = buffer;
  auto header = encode_header(BUSINESS_MESSAGE_REJECT);
  header->body_fields_presence_map[0] = 0b10100000;

  encode(msg.business_reject_code);
  if (msg.reason.len > 0) {
    set_presence(header->body_fields_presence_map, 1);
    encode(msg.reason);
  }
  encode(msg.reference_message_type);
  if (msg.reference_field_name[0] != 0) {
    set_presence(header->body_fields_presence_map, 3);
    encode(msg.reference_field_name);
  }
  if (msg.reference_sequence_number > 0) {
    set_presence(header->body_fields_presence_map, 4);
    encode(msg.reference_sequence_number);
  }
  if (msg.business_reject_reference_id[0] != 0) {
    set_presence(header->body_fields_presence_map, 5);
    encode(msg.business_reject_reference_id);
  }

  fill_header_and_trailer();
}

void OcgEncoder::encode_msg(const LookupResponse& rsp, MsgBuffer* buffer) {
  buffer->size = 0;
  cur_msg_buf_ = buffer;
  auto header = encode_header(LOOKUP_RESPONSE);
  header->sequence_number = 1;
  header->body_fields_presence_map[0] = 0b10000000;

  encode(rsp.status);
  if (rsp.status != LOOKUP_SERVICE_ACCEPTED) {
    set_presence(header->body_fields_presence_map, 1);
    encode(rsp.lookup_reject_code);
  }
  if (rsp.reason.len > 0) {
    set_presence(header->body_fields_presence_map, 2);
    encode(rsp.reason);
  }
  if (rsp.status == LOOKUP_SERVICE_ACCEPTED) {
    set_presence(header->body_fields_presence_map, 3);
    set_presence(header->body_fields_presence_map, 4);
    set_presence(header->body_fields_presence_map, 5);
    set_presence(header->body_fields_presence_map, 6);
    encode(rsp.primary_ip);
    encode(rsp.primary_port);
    encode(rsp.secondary_ip);
    encode(rsp.secondary_port);
  }

  fill_header_and_trailer();
}
