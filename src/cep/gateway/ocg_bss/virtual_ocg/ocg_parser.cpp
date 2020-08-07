// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "virtual_ocg/ocg_parser.h"

#include "protocol/protocol_parser.h"

using namespace ft::bss;

OcgParser::OcgParser() { buffer_.resize(1024 * 1024 * 8); }

int OcgParser::parse_raw_data(std::size_t new_data_size) {
  int ret = 0;
  std::size_t readable_size;

  assert(new_data_size <= buffer_.size() &&
         new_data_size + write_head_ <= buffer_.size());
  write_head_ += new_data_size;

  for (;;) {
    readable_size = readable();
    if (readable_size < sizeof(MessageHeader)) break;

    auto header = reinterpret_cast<MessageHeader*>(readable_start());
    if (readable_size < header->length) break;

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
      case NEW_ORDER: {
        NewOrderRequest new_order_req{};
        parse_new_order_request(*header, reinterpret_cast<char*>(header + 1),
                                &new_order_req);
        handler_->on_new_order_request(header, &new_order_req);
        break;
      }
      case AMEND_REQUEST: {
        AmendRequest amend_req{};
        parse_amend_request(*header, reinterpret_cast<char*>(header + 1),
                            &amend_req);
        handler_->on_amend_request(header, &amend_req);
        break;
      }
      case CANCEL_REQUEST: {
        CancelRequest cancel_req{};
        parse_cancel_request(*header, reinterpret_cast<char*>(header + 1),
                             &cancel_req);
        handler_->on_cancel_request(header, &cancel_req);
        break;
      }
      case MASS_CANCEL_REQUEST: {
        MassCancelRequest mass_cancel_req{};
        parse_mass_cancel_request(*header, reinterpret_cast<char*>(header + 1),
                                  &mass_cancel_req);
        handler_->on_mass_cancel_request(header, &mass_cancel_req);
        break;
      }
      case OBO_CANCEL_REQUEST: {
        OboCancelRequest obo_cancel_req{};
        parse_obo_cancel_request(*header, reinterpret_cast<char*>(header + 1),
                                 &obo_cancel_req);
        handler_->on_obo_cancel_request(header, &obo_cancel_req);
        break;
      }
      case OBO_MASS_CANCEL_REQUEST: {
        OboMassCancelRequest obo_mass_cancel_req{};
        parse_obo_mass_cancel_request(
            *header, reinterpret_cast<char*>(header + 1), &obo_mass_cancel_req);
        handler_->on_obo_mass_cancel_request(header, &obo_mass_cancel_req);
        break;
      }
      case QUOTE: {
        QuoteRequest quote_req{};
        parse_quote_request(*header, reinterpret_cast<char*>(header + 1),
                            &quote_req);
        handler_->on_quote_request(header, &quote_req);
        break;
      }
      case QUOTE_CANCEL: {
        QuoteCancelRequest quote_cancel_req{};
        parse_quote_cancel_request(*header, reinterpret_cast<char*>(header + 1),
                                   &quote_cancel_req);
        handler_->on_quote_cancel_request(header, &quote_cancel_req);
        break;
      }
      case PARTY_ENTITLEMENTS_REQUEST: {
        PartyEntitlementRequest party_entitlement_req{};
        parse_party_entitlement_request(*header,
                                        reinterpret_cast<char*>(header + 1),
                                        &party_entitlement_req);
        handler_->on_party_entitlement_request(header, &party_entitlement_req);
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

  if (writable() < 1024 * 16) {
    memmove(buffer_.data(), readable_start(), readable_size);
    read_head_ = 0;
    write_head_ = readable_size;
  }

  return ret;
}

const char* parse_lookup_request(const MessageHeader& header, const char* p,
                                 LookupRequest* req) {
  p = decode(p, req->type_of_service);
  p = decode(p, req->protocol_type);

  return p;
}

const char* parse_new_order_request(const MessageHeader& header, const char* p,
                                    NewOrderRequest* req) {
  p = decode(p, req->client_order_id);
  p = decode(p, req->submitting_broker_id);
  p = decode(p, req->security_id);
  p = decode(p, req->security_id_source);
  if (is_present(header.body_fields_presence_map, 4))
    p = decode(p, req->security_exchange);
  if (is_present(header.body_fields_presence_map, 5))
    p = decode(p, req->broker_location_id);
  p = decode(p, req->transaction_time);
  p = decode(p, req->side);
  p = decode(p, req->order_type);
  if (is_present(header.body_fields_presence_map, 9)) p = decode(p, req->price);
  p = decode(p, req->order_quantity);
  if (is_present(header.body_fields_presence_map, 11)) p = decode(p, req->tif);
  if (is_present(header.body_fields_presence_map, 12))
    p = decode(p, req->position_effect);
  if (is_present(header.body_fields_presence_map, 13))
    p = decode(p, req->order_restrictions);
  if (is_present(header.body_fields_presence_map, 14))
    p = decode(p, req->max_price_levels);
  if (is_present(header.body_fields_presence_map, 15))
    p = decode(p, req->order_capacity);
  if (is_present(header.body_fields_presence_map, 16)) p = decode(p, req->text);
  if (is_present(header.body_fields_presence_map, 17))
    p = decode(p, req->execution_instructions);
  if (is_present(header.body_fields_presence_map, 18))
    p = decode(p, req->disclosure_instructions);
  if (is_present(header.body_fields_presence_map, 19))
    p = decode(p, req->lot_type);

  return p;
}

const char* parse_amend_request(const MessageHeader& header, const char* p,
                                AmendRequest* req) {
  p = decode(p, req->client_order_id);
  p = decode(p, req->submitting_broker_id);
  p = decode(p, req->security_id);
  p = decode(p, req->security_id_source);
  if (is_present(header.body_fields_presence_map, 4))
    p = decode(p, req->security_exchange);
  if (is_present(header.body_fields_presence_map, 5))
    p = decode(p, req->broker_location_id);
  p = decode(p, req->transaction_time);
  p = decode(p, req->side);
  p = decode(p, req->original_client_order_id);
  if (is_present(header.body_fields_presence_map, 9))
    p = decode(p, req->order_id);
  p = decode(p, req->order_type);
  if (is_present(header.body_fields_presence_map, 11))
    p = decode(p, req->price);
  if (is_present(header.body_fields_presence_map, 12))
    p = decode(p, req->order_quantity);
  if (is_present(header.body_fields_presence_map, 13)) p = decode(p, req->tif);
  if (is_present(header.body_fields_presence_map, 14))
    p = decode(p, req->position_effect);
  if (is_present(header.body_fields_presence_map, 15))
    p = decode(p, req->order_restrictions);
  if (is_present(header.body_fields_presence_map, 16))
    p = decode(p, req->max_price_levels);
  if (is_present(header.body_fields_presence_map, 17))
    p = decode(p, req->order_capacity);
  if (is_present(header.body_fields_presence_map, 18)) p = decode(p, req->text);
  if (is_present(header.body_fields_presence_map, 19))
    p = decode(p, req->execution_instructions);
  p = decode(p, req->disclosure_instructions);

  return p;
}

const char* parse_cancel_request(const MessageHeader& header, const char* p,
                                 CancelRequest* req) {
  p = decode(p, req->client_order_id);
  p = decode(p, req->submitting_broker_id);
  p = decode(p, req->security_id);
  p = decode(p, req->security_id_source);
  if (is_present(header.body_fields_presence_map, 4))
    p = decode(p, req->security_exchange);
  if (is_present(header.body_fields_presence_map, 5))
    p = decode(p, req->broker_location_id);
  p = decode(p, req->transaction_time);
  p = decode(p, req->side);
  p = decode(p, req->original_client_order_id);
  if (is_present(header.body_fields_presence_map, 9))
    p = decode(p, req->order_id);
  if (is_present(header.body_fields_presence_map, 10)) p = decode(p, req->text);

  return p;
}

const char* parse_mass_cancel_request(const MessageHeader& header,
                                      const char* p, MassCancelRequest* req) {
  p = decode(p, req->client_order_id);
  p = decode(p, req->submitting_broker_id);
  if (is_present(header.body_fields_presence_map, 2))
    p = decode(p, req->security_id);
  if (is_present(header.body_fields_presence_map, 3))
    p = decode(p, req->security_id_source);
  if (is_present(header.body_fields_presence_map, 4))
    p = decode(p, req->security_exchange);
  if (is_present(header.body_fields_presence_map, 5))
    p = decode(p, req->broker_location_id);
  p = decode(p, req->transaction_time);
  if (is_present(header.body_fields_presence_map, 7)) p = decode(p, req->side);
  p = decode(p, req->mass_cancel_request_type);
  if (is_present(header.body_fields_presence_map, 9))
    p = decode(p, req->market_segment_id);

  return p;
}

const char* parse_obo_cancel_request(const MessageHeader& header, const char* p,
                                     OboCancelRequest* req) {
  p = decode(p, req->client_order_id);
  p = decode(p, req->submitting_broker_id);
  p = decode(p, req->security_id);
  p = decode(p, req->security_id_source);
  if (is_present(header.body_fields_presence_map, 4))
    p = decode(p, req->security_exchange);
  if (is_present(header.body_fields_presence_map, 5))
    p = decode(p, req->broker_location_id);
  p = decode(p, req->transaction_time);
  p = decode(p, req->side);
  if (is_present(header.body_fields_presence_map, 8))
    p = decode(p, req->original_client_order_id);
  p = decode(p, req->order_id);
  p = decode(p, req->owning_broker_id);
  if (is_present(header.body_fields_presence_map, 11)) p = decode(p, req->text);

  return p;
}

const char* parse_obo_mass_cancel_request(const MessageHeader& header,
                                          const char* p,
                                          OboMassCancelRequest* req) {
  p = decode(p, req->client_order_id);
  p = decode(p, req->submitting_broker_id);
  if (is_present(header.body_fields_presence_map, 2))
    p = decode(p, req->security_id);
  if (is_present(header.body_fields_presence_map, 3))
    p = decode(p, req->security_id_source);
  if (is_present(header.body_fields_presence_map, 4))
    p = decode(p, req->security_exchange);
  if (is_present(header.body_fields_presence_map, 5))
    p = decode(p, req->broker_location_id);
  p = decode(p, req->transaction_time);
  if (is_present(header.body_fields_presence_map, 7)) p = decode(p, req->side);
  p = decode(p, req->mass_cancel_request_type);
  if (is_present(header.body_fields_presence_map, 9))
    p = decode(p, req->market_segment_id);
  p = decode(p, req->owning_broker_id);

  return p;
}

const char* parse_quote_request(const MessageHeader& header, const char* p,
                                QuoteRequest* req) {
  p = decode(p, req->submitting_broker_id);
  if (is_present(header.body_fields_presence_map, 1))
    p = decode(p, req->broker_location_id);
  p = decode(p, req->security_id);
  p = decode(p, req->security_id_source);
  if (is_present(header.body_fields_presence_map, 4))
    p = decode(p, req->security_exchange);
  p = decode(p, req->quote_bid_id);
  p = decode(p, req->quote_offer_id);
  if (is_present(header.body_fields_presence_map, 7))
    p = decode(p, req->quote_type);
  if (is_present(header.body_fields_presence_map, 8)) p = decode(p, req->side);
  p = decode(p, req->bid_size);
  p = decode(p, req->offer_size);
  p = decode(p, req->bid_price);
  p = decode(p, req->offer_price);
  p = decode(p, req->transaction_time);
  if (is_present(header.body_fields_presence_map, 14))
    p = decode(p, req->position_effect);
  if (is_present(header.body_fields_presence_map, 15))
    p = decode(p, req->order_restrictions);
  if (is_present(header.body_fields_presence_map, 16)) p = decode(p, req->text);
  if (is_present(header.body_fields_presence_map, 17))
    p = decode(p, req->execution_instructions);

  return p;
}

const char* parse_quote_cancel_request(const MessageHeader& header,
                                       const char* p, QuoteCancelRequest* req) {
  p = decode(p, req->submitting_broker_id);
  if (is_present(header.body_fields_presence_map, 1))
    p = decode(p, req->broker_location_id);
  if (is_present(header.body_fields_presence_map, 2))
    p = decode(p, req->security_id);
  if (is_present(header.body_fields_presence_map, 3))
    p = decode(p, req->security_id_source);
  if (is_present(header.body_fields_presence_map, 4))
    p = decode(p, req->security_exchange);
  p = decode(p, req->quote_message_id);
  p = decode(p, req->quote_cancel_type);

  return p;
}

const char* parse_party_entitlement_request(const MessageHeader& header,
                                            const char* p,
                                            PartyEntitlementRequest* req) {
  p = decode(p, req->entitlement_request_id);

  return p;
}