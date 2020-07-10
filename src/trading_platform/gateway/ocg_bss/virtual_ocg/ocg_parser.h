// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef BSS_VIRTUAL_OCG_OCG_PARSER_H_
#define BSS_VIRTUAL_OCG_OCG_PARSER_H_

#include <vector>

#include "protocol/protocol.h"
#include "virtual_ocg/ocg_handler.h"

using namespace ft::bss;

/*
 * 模拟OCG，解析从BSS发来的消息
 */
class OcgParser {
 public:
  OcgParser();

  void set_handler(OcgHandler* handler) { handler_ = handler; }

  void clear() {
    write_head_ = 0;
    read_head_ = 0;
  }

  int parse_raw_data(std::size_t new_data_size);

  std::size_t readable() const { return write_head_ - read_head_; }

  std::size_t writable() const { return buffer_.size() - write_head_; }

  char* writable_start() { return buffer_.data() + write_head_; }

  char* readable_start() { return buffer_.data() + read_head_; }

 private:
  std::vector<char> buffer_;
  std::size_t read_head_ = 0;
  std::size_t write_head_ = 0;

  OcgHandler* handler_ = nullptr;
};

const char* parse_lookup_request(const MessageHeader& header, const char* p,
                                 LookupRequest* req);

const char* parse_new_order_request(const MessageHeader& header, const char* p,
                                    NewOrderRequest* req);

const char* parse_amend_request(const MessageHeader& header, const char* p,
                                AmendRequest* req);

const char* parse_cancel_request(const MessageHeader& header, const char* p,
                                 CancelRequest* req);

const char* parse_mass_cancel_request(const MessageHeader& header,
                                      const char* p, MassCancelRequest* req);

const char* parse_obo_cancel_request(const MessageHeader& header, const char* p,
                                     OboCancelRequest* req);

const char* parse_obo_mass_cancel_request(const MessageHeader& header,
                                          const char* p,
                                          OboMassCancelRequest* req);

const char* parse_quote_request(const MessageHeader& header, const char* p,
                                QuoteRequest* req);

const char* parse_quote_cancel_request(const MessageHeader& header,
                                       const char* p, QuoteCancelRequest* req);

const char* parse_party_entitlement_request(const MessageHeader& header,
                                            const char* p,
                                            PartyEntitlementRequest* req);

#endif  // BSS_VIRTUAL_OCG_OCG_PARSER_H_
