// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "protocol/protocol_parser.h"

#include <cstring>

namespace ft::bss {

BinaryMessageDecoder::BinaryMessageDecoder() {
  buffer_.resize(1024 * 1024 * 8);
}

void BinaryMessageDecoder::set_handler(MessageHandler* handler) {
  assert(handler);
  handler_ = handler;
}

int BinaryMessageDecoder::parse_raw_data(std::size_t new_data_size) {
  int ret = 0;
  std::size_t rd_size;

  assert(new_data_size <= buffer_.size() &&
         new_data_size + write_head_ <= buffer_.size());
  write_head_ += new_data_size;

  for (;;) {
    rd_size = readable_size();
    if (rd_size < sizeof(MessageHeader)) break;

    auto header = reinterpret_cast<MessageHeader*>(readable_start());
    if (rd_size < header->length) break;

    if (!crc32c_check(header)) {
      handler_->on_invalid_msg();
      return -1;
    }

    switch (header->message_type) {
      case HEARTBEAT: {
        HeartbeatMessage hearbeat_msg{};
        parse_heartbeat_message(*header, reinterpret_cast<char*>(header + 1),
                                &hearbeat_msg);
        handler_->on_heartbeat_msg(header, &hearbeat_msg);
        break;
      }
      case TEST_REQUEST: {
        TestRequest test_req{};
        parse_test_request(*header, reinterpret_cast<char*>(header + 1),
                           &test_req);
        handler_->on_test_request(header, &test_req);
        break;
      }
      case RESEND_REQUEST: {
        ResendRequest resend_req{};
        parse_resend_request(*header, reinterpret_cast<char*>(header + 1),
                             &resend_req);
        handler_->on_resend_request(header, &resend_req);
        break;
      }
      case REJECT: {
        RejectMessage reject_msg{};
        parse_reject_message(*header, reinterpret_cast<char*>(header + 1),
                             &reject_msg);
        handler_->on_reject_msg(header, &reject_msg);
        break;
      }
      case SEQUENCE_RESET: {
        SequenceResetMessage seq_reset_msg{};
        parse_sequence_reset_message(
            *header, reinterpret_cast<char*>(header + 1), &seq_reset_msg);
        handler_->on_sequence_reset_msg(header, &seq_reset_msg);
        break;
      }
      case LOGON: {
        LogonMessage logon_msg{};
        parse_logon_message(*header, reinterpret_cast<char*>(header + 1),
                            &logon_msg);
        handler_->on_logon_msg(header, &logon_msg);
        break;
      }
      case LOGOUT: {
        LogoutMessage logout_msg{};
        parse_logout_message(*header, reinterpret_cast<char*>(header + 1),
                             &logout_msg);
        handler_->on_logout_msg(header, &logout_msg);
        break;
      }
      case BUSINESS_MESSAGE_REJECT: {
        BusinessRejectMessage business_reject_msg{};
        parse_business_reject_message(
            *header, reinterpret_cast<char*>(header + 1), &business_reject_msg);
        handler_->on_business_reject_msg(header, &business_reject_msg);
        break;
      }
      case EXECUTION_REPORT: {
        ExecutionReport execution_report{};
        parse_execution_report(*header, reinterpret_cast<char*>(header + 1),
                               &execution_report);
        handler_->on_execution_report(header, &execution_report);
        break;
      }
      case ORDER_MASS_CANCEL_REPORT: {
        OrderMassCancelReport mass_cancel_report{};
        parse_mass_cancel_report(*header, reinterpret_cast<char*>(header + 1),
                                 &mass_cancel_report);
        handler_->on_mass_cancel_report(header, &mass_cancel_report);
        break;
      }
      case QUOTE_STATUS_REPORT: {
        QuoteStatusReport quote_status_report{};
        parse_quote_status_report(*header, reinterpret_cast<char*>(header + 1),
                                  &quote_status_report);
        handler_->on_quote_status_report(header, &quote_status_report);
        break;
      }
      case TRADE_CAPTURE_REPORT: {
        TradeCaptureReport trade_capture_report{};
        parse_trade_capture_report(*header, reinterpret_cast<char*>(header + 1),
                                   &trade_capture_report);
        handler_->on_trade_capture_report(header, &trade_capture_report);
        break;
      }
      case TRADE_CAPTURE_REPORT_ACK: {
        TradeCaptureReportAck trade_capture_report_ack{};
        parse_trade_capture_report_ack(*header,
                                       reinterpret_cast<char*>(header + 1),
                                       &trade_capture_report_ack);
        handler_->on_trade_capture_report_ack(header,
                                              &trade_capture_report_ack);
        break;
      }
      case THROTTLE_ENTITLEMENT_RESPONSE: {
        break;
      }
      case PARTY_ENTITLEMENTS_REPORT: {
        break;
      }
      default: {
        handler_->on_invalid_msg();
        return -1;
      }
    }

    read_head_ += header->length;
    ++ret;
  }

  if (writable_size() < 1024 * 16) {
    memmove(buffer_.data(), readable_start(), rd_size);
    read_head_ = 0;
    write_head_ = rd_size;
  }

  return ret;
}

const char* parse_lookup_response(const MessageHeader& header, const char* p,
                                  LookupResponse* rsp) {
  p = decode(p, rsp->status);
  if (is_present(header.body_fields_presence_map, 1))
    p = decode(p, rsp->lookup_reject_code);
  if (is_present(header.body_fields_presence_map, 2))
    p = decode(p, rsp->reason);
  if (is_present(header.body_fields_presence_map, 3))
    p = decode(p, rsp->primary_ip);
  if (is_present(header.body_fields_presence_map, 4))
    p = decode(p, rsp->primary_port);
  if (is_present(header.body_fields_presence_map, 5))
    p = decode(p, rsp->secondary_ip);
  if (is_present(header.body_fields_presence_map, 6))
    p = decode(p, rsp->secondary_port);

  return p;
}

const char* parse_logon_message(const MessageHeader& header, const char* p,
                                LogonMessage* msg) {
  if (is_present(header.body_fields_presence_map, 0))
    p = decode(p, msg->password);
  if (is_present(header.body_fields_presence_map, 1))
    p = decode(p, msg->new_password);
  p = decode(p, msg->next_expected_message_sequence);
  if (is_present(header.body_fields_presence_map, 3))
    p = decode(p, msg->session_status);
  if (is_present(header.body_fields_presence_map, 4)) p = decode(p, msg->text);
  if (is_present(header.body_fields_presence_map, 5))
    p = decode(p, msg->test_message_indicator);

  return p;
}

const char* parse_logout_message(const MessageHeader& header, const char* p,
                                 LogoutMessage* msg) {
  if (is_present(header.body_fields_presence_map, 0))
    p = decode(p, msg->logout_text);
  if (is_present(header.body_fields_presence_map, 1))
    p = decode(p, msg->session_status);

  return p;
}

const char* parse_heartbeat_message(const MessageHeader& header, const char* p,
                                    HeartbeatMessage* msg) {
  if (is_present(header.body_fields_presence_map, 0))
    p = decode(p, msg->reference_test_request_id);

  return p;
}

const char* parse_test_request(const MessageHeader& header, const char* p,
                               TestRequest* msg) {
  p = decode(p, msg->test_request_id);

  return p;
}

const char* parse_resend_request(const MessageHeader& header, const char* p,
                                 ResendRequest* msg) {
  p = decode(p, msg->start_sequence);
  p = decode(p, msg->end_sequence);

  return p;
}

const char* parse_reject_message(const MessageHeader& header, const char* p,
                                 RejectMessage* msg) {
  p = decode(p, msg->message_reject_code);
  if (is_present(header.body_fields_presence_map, 1))
    p = decode(p, msg->reason);
  if (is_present(header.body_fields_presence_map, 2))
    p = decode(p, msg->reference_message_type);
  if (is_present(header.body_fields_presence_map, 3))
    p = decode(p, msg->reference_field_name);
  p = decode(p, msg->reference_sequence_number);
  if (is_present(header.body_fields_presence_map, 5))
    p = decode(p, msg->client_order_id);

  return p;
}

const char* parse_sequence_reset_message(const MessageHeader& header,
                                         const char* p,
                                         SequenceResetMessage* msg) {
  if (is_present(header.body_fields_presence_map, 0))
    p = decode(p, msg->gap_fill);
  p = decode(p, msg->new_sequence_number);

  return p;
}

const char* parse_execution_report(const MessageHeader& header, const char* p,
                                   ExecutionReport* report) {
  p = decode(p, report->client_order_id);
  p = decode(p, report->submitting_broker_id);
  p = decode(p, report->security_id);
  p = decode(p, report->security_id_source);
  if (is_present(header.body_fields_presence_map, 4))
    p = decode(p, report->security_exchange);
  if (is_present(header.body_fields_presence_map, 5))
    p = decode(p, report->broker_location_id);
  p = decode(p, report->transaction_time);
  p = decode(p, report->side);
  if (is_present(header.body_fields_presence_map, 8))
    p = decode(p, report->original_client_order_id);
  p = decode(p, report->order_id);
  if (is_present(header.body_fields_presence_map, 10))
    p = decode(p, report->owning_broker_id);
  if (is_present(header.body_fields_presence_map, 11))
    p = decode(p, report->order_type);
  if (is_present(header.body_fields_presence_map, 12))
    p = decode(p, report->price);
  if (is_present(header.body_fields_presence_map, 13))
    p = decode(p, report->order_quantity);
  if (is_present(header.body_fields_presence_map, 14))
    p = decode(p, report->tif);
  if (is_present(header.body_fields_presence_map, 15))
    p = decode(p, report->position_effect);
  if (is_present(header.body_fields_presence_map, 16))
    p = decode(p, report->order_restrictions);
  if (is_present(header.body_fields_presence_map, 17))
    p = decode(p, report->max_price_levels);
  if (is_present(header.body_fields_presence_map, 18))
    p = decode(p, report->order_capacity);
  if (is_present(header.body_fields_presence_map, 19))
    p = decode(p, report->text);
  if (is_present(header.body_fields_presence_map, 20))
    p = decode(p, report->reason);
  p = decode(p, report->execution_id);
  p = decode(p, report->order_status);
  p = decode(p, report->exec_type);
  p = decode(p, report->cumulative_quantity);
  p = decode(p, report->leaves_quantity);

  switch (report->exec_type) {
    case EXEC_TYPE_NEW:
      if (is_present(header.body_fields_presence_map, 27))
        p = decode(p, report->lot_type);
      break;
    case EXEC_TYPE_CANCEL:
      if (is_present(header.body_fields_presence_map, 28))
        p = decode(p, report->exec_restatement_reason);
      break;
    case EXEC_TYPE_AMEND:
      break;
    case EXEC_TYPE_REJECT:
      if (is_present(header.body_fields_presence_map, 26))
        p = decode(p, report->order_reject_code);
      break;
    case EXEC_TYPE_EXPIRE:
      break;
    case EXEC_TYPE_TRADE:
      if (is_present(header.body_fields_presence_map, 27))
        p = decode(p, report->lot_type);
      if (is_present(header.body_fields_presence_map, 30))
        p = decode(p, report->match_type);
      if (is_present(header.body_fields_presence_map, 31))
        p = decode(p, report->counterparty_broker_id);
      p = decode(p, report->execution_quantity);
      p = decode(p, report->execution_price);
      if (is_present(header.body_fields_presence_map, 34))
        p = decode(p, report->reference_execution_id);
      if (is_present(header.body_fields_presence_map, 35))
        p = decode(p, report->order_category);
      if (is_present(header.body_fields_presence_map, 38))
        p = decode(p, report->trade_match_id);
      if (is_present(header.body_fields_presence_map, 39))
        p = decode(p, report->exchange_trade_type);
      break;
    case EXEC_TYPE_TRADE_CANCEL:
      if (is_present(header.body_fields_presence_map, 28))
        p = decode(p, report->exec_restatement_reason);
      if (is_present(header.body_fields_presence_map, 30))
        p = decode(p, report->execution_quantity);
      if (is_present(header.body_fields_presence_map, 31))
        p = decode(p, report->execution_price);
      p = decode(p, report->reference_execution_id);
      if (is_present(header.body_fields_presence_map, 35))
        p = decode(p, report->order_category);
      break;
    case EXEC_TYPE_CANCEL_REJECT:
      if (is_present(header.body_fields_presence_map, 29))
        p = decode(p, report->cancel_reject_code);
      break;
    case EXEC_TYPE_AMEND_REJECT:
      if (is_present(header.body_fields_presence_map, 36))
        p = decode(p, report->amend_reject_code);
      break;
    default:
      assert(false);
  }

  return p;
}

const char* parse_mass_cancel_report(const MessageHeader& header, const char* p,
                                     OrderMassCancelReport* report) {
  if (is_present(header.body_fields_presence_map, 0))
    p = decode(p, report->client_order_id);
  if (is_present(header.body_fields_presence_map, 1))
    p = decode(p, report->submitting_broker_id);
  if (is_present(header.body_fields_presence_map, 2))
    p = decode(p, report->security_id);
  if (is_present(header.body_fields_presence_map, 3))
    p = decode(p, report->security_id_source);
  if (is_present(header.body_fields_presence_map, 4))
    p = decode(p, report->security_exchange);
  if (is_present(header.body_fields_presence_map, 5))
    p = decode(p, report->broker_location_id);
  p = decode(p, report->transaction_time);
  p = decode(p, report->mass_cancel_request_type);
  if (is_present(header.body_fields_presence_map, 8))
    p = decode(p, report->owning_broker_id);
  p = decode(p, report->mass_action_report_id);
  p = decode(p, report->mass_cancel_response);
  if (is_present(header.body_fields_presence_map, 11))
    p = decode(p, report->mass_cancel_reject_code);
  if (is_present(header.body_fields_presence_map, 12))
    p = decode(p, report->reason);

  return p;
}

const char* parse_trade_capture_report(const MessageHeader& header,
                                       const char* p,
                                       TradeCaptureReport* report) {
  if (is_present(header.body_fields_presence_map, 0))
    p = decode(p, report->trade_report_id);
  if (is_present(header.body_fields_presence_map, 1))
    p = decode(p, report->trade_report_trans_type);
  p = decode(p, report->trade_report_type);
  if (is_present(header.body_fields_presence_map, 3))
    p = decode(p, report->trade_handling_instructions);
  p = decode(p, report->submitting_broker_id);
  if (is_present(header.body_fields_presence_map, 5))
    p = decode(p, report->counterparty_broker_id);
  if (is_present(header.body_fields_presence_map, 6))
    p = decode(p, report->broker_location_id);
  p = decode(p, report->security_id);
  p = decode(p, report->security_id_source);
  if (is_present(header.body_fields_presence_map, 9))
    p = decode(p, report->security_exchange);
  p = decode(p, report->side);
  p = decode(p, report->transaction_time);
  if (is_present(header.body_fields_presence_map, 12))
    p = decode(p, report->trade_id);
  if (is_present(header.body_fields_presence_map, 13))
    p = decode(p, report->trade_type);
  p = decode(p, report->execution_quantity);
  p = decode(p, report->execution_price);
  if (is_present(header.body_fields_presence_map, 16))
    p = decode(p, report->clearing_instruction);
  if (is_present(header.body_fields_presence_map, 17))
    p = decode(p, report->position_effect);
  if (is_present(header.body_fields_presence_map, 19))
    p = decode(p, report->order_capacity);
  if (is_present(header.body_fields_presence_map, 21))
    p = decode(p, report->text);
  if (is_present(header.body_fields_presence_map, 22))
    p = decode(p, report->execution_instructions);
  if (is_present(header.body_fields_presence_map, 23))
    p = decode(p, report->exec_type);
  if (is_present(header.body_fields_presence_map, 24))
    p = decode(p, report->trade_report_status);
  if (is_present(header.body_fields_presence_map, 25))
    p = decode(p, report->exchange_trade_type);
  if (is_present(header.body_fields_presence_map, 27))
    p = decode(p, report->order_id);

  return p;
}

const char* parse_trade_capture_report_ack(const MessageHeader& header,
                                           const char* p,
                                           TradeCaptureReportAck* ack) {
  p = decode(p, ack->trade_report_id);
  if (is_present(header.body_fields_presence_map, 1))
    p = decode(p, ack->trade_report_trans_type);
  p = decode(p, ack->trade_report_type);
  if (is_present(header.body_fields_presence_map, 3))
    p = decode(p, ack->trade_handling_instructions);
  p = decode(p, ack->submitting_broker_id);
  if (is_present(header.body_fields_presence_map, 5))
    p = decode(p, ack->counterparty_broker_id);
  if (is_present(header.body_fields_presence_map, 6))
    p = decode(p, ack->broker_location_id);
  p = decode(p, ack->security_id);
  p = decode(p, ack->security_id_source);
  if (is_present(header.body_fields_presence_map, 9))
    p = decode(p, ack->security_exchange);
  p = decode(p, ack->side);
  p = decode(p, ack->transaction_time);
  if (is_present(header.body_fields_presence_map, 12))
    p = decode(p, ack->trade_id);
  if (is_present(header.body_fields_presence_map, 13))
    p = decode(p, ack->trade_report_status);
  if (is_present(header.body_fields_presence_map, 14))
    p = decode(p, ack->trade_report_reject_code);
  if (is_present(header.body_fields_presence_map, 15))
    p = decode(p, ack->reason);

  return p;
}

const char* parse_business_reject_message(const MessageHeader& header,
                                          const char* p,
                                          BusinessRejectMessage* msg) {
  p = decode(p, msg->business_reject_code);
  if (is_present(header.body_fields_presence_map, 1))
    p = decode(p, msg->reason);
  p = decode(p, msg->reference_message_type);
  if (is_present(header.body_fields_presence_map, 3))
    p = decode(p, msg->reference_field_name);
  if (is_present(header.body_fields_presence_map, 4))
    p = decode(p, msg->reference_sequence_number);
  if (is_present(header.body_fields_presence_map, 5))
    p = decode(p, msg->business_reject_reference_id);

  return p;
}

const char* parse_quote_status_report(const MessageHeader& header,
                                      const char* p,
                                      QuoteStatusReport* report) {
  p = decode(p, report->submitting_broker_id);
  if (is_present(header.body_fields_presence_map, 1))
    p = decode(p, report->broker_location_id);
  if (is_present(header.body_fields_presence_map, 2))
    p = decode(p, report->security_id);
  if (is_present(header.body_fields_presence_map, 3))
    p = decode(p, report->security_id_source);
  if (is_present(header.body_fields_presence_map, 4))
    p = decode(p, report->security_exchange);
  if (is_present(header.body_fields_presence_map, 5))
    p = decode(p, report->quote_bid_id);
  if (is_present(header.body_fields_presence_map, 6))
    p = decode(p, report->quote_offer_id);
  if (is_present(header.body_fields_presence_map, 7))
    p = decode(p, report->quote_type);
  p = decode(p, report->transaction_time);
  if (is_present(header.body_fields_presence_map, 9))
    p = decode(p, report->quote_message_id);
  if (is_present(header.body_fields_presence_map, 10))
    p = decode(p, report->quote_cancel_type);
  if (is_present(header.body_fields_presence_map, 11))
    p = decode(p, report->quote_status);
  if (is_present(header.body_fields_presence_map, 12))
    p = decode(p, report->quote_reject_code);
  if (is_present(header.body_fields_presence_map, 13))
    p = decode(p, report->reason);

  return p;
}

}  // namespace ft
