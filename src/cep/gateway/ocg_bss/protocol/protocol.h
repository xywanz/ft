// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef OCG_BSS_PROTOCOL_H_
#define OCG_BSS_PROTOCOL_H_

#include <cstdint>

namespace ft::bss {

enum OcgMessageType : uint8_t {
  HEARTBEAT = 0,
  TEST_REQUEST = 1,
  RESEND_REQUEST = 2,
  REJECT = 3,
  SEQUENCE_RESET = 4,
  LOGON = 5,
  LOGOUT = 6,
  LOOKUP_REQUEST = 7,
  LOOKUP_RESPONSE = 8,
  BUSINESS_MESSAGE_REJECT = 9,
  EXECUTION_REPORT = 10,
  NEW_ORDER = 11,
  AMEND_REQUEST = 12,
  CANCEL_REQUEST = 13,
  MASS_CANCEL_REQUEST = 14,
  ORDER_MASS_CANCEL_REPORT = 15,
  QUOTE = 16,
  QUOTE_CANCEL = 17,
  QUOTE_STATUS_REPORT = 18,
  TRADE_CAPTURE_REPORT = 21,
  TRADE_CAPTURE_REPORT_ACK = 22,
  OBO_CANCEL_REQUEST = 23,
  OBO_MASS_CANCEL_REQUEST = 24,
  THROTTLE_ENTITLEMENT_REQUEST = 25,
  THROTTLE_ENTITLEMENT_RESPONSE = 26,
  PARTY_ENTITLEMENTS_REQUEST = 27,
  PARTY_ENTITLEMENTS_REPORT = 28
};

enum OcgOrderType : uint8_t {
  ORDER_TYPE_MARKET = 1,
  ORDER_TYPE_LIMIT = 2,
};

enum OcgLotType : uint8_t {
  LOT_TYPE_BOARD = 0,
  LOT_TYPE_ODD_SPEC = 1,
};

enum OcgExecType : int8_t {
  EXEC_TYPE_NEW = '0',
  EXEC_TYPE_CANCEL = '4',
  EXEC_TYPE_AMEND = '5',
  EXEC_TYPE_REJECT = '8',
  EXEC_TYPE_EXPIRE = 'C',
  EXEC_TYPE_TRADE = 'F',
  EXEC_TYPE_TRADE_CANCEL = 'H',
  EXEC_TYPE_CANCEL_REJECT = 'X',
  EXEC_TYPE_AMEND_REJECT = 'Y'
};

enum MessageRejectCode : uint16_t {
  MSG_REJECT_CODE_REQUIRED_FIELD_MISSING = 1,
  MSG_REJECT_CODE_FILED_NOT_DEFINED_FOR_THIS_MESSAGE_TYPE = 2,
  MSG_REJECT_CODE_UNDIFINED_FIELD = 3,
  MSG_REJECT_CODE_FIELD_SEPECIFIED_WITHOUT_A_VALUE = 4,
  MSG_REJECT_CODE_VALUE_IS_INCORRECT_FOR_THIS_FIELD = 5,
  MSG_REJECT_CODE_COMP_ID_PROBLEM = 9,
  MSG_REJECT_CODE_INVALID_MESSAGE_TYPE = 11,
  MSG_REJECT_CODE_FIELD_APPEARS_MORE_THAN_ONCE = 13,
  MSG_REJECT_CODE_OTHER = 99
};

enum LookupServiceStatus {
  LOOKUP_SERVICE_REJECTED = 0,
  LOOKUP_SERVICE_ACCEPTED = 1
};

enum BssSessionStatus {
  SESSION_ACTIVE = 0,
  SESSION_PASSWORD_CHANGE,
  SESSION_PASSWORD_DUE_TO_EXPIRE,
  NEW_SESSION_PASSWORD_NOT_COMPLY_WITH_THE_POLICY,
  SESSION_LOGOUT_COMPLETE,
  INVAILD_USERNAME_OR_PASSWORD,
  ACCOUNT_LOCKED,
  LOGONS_NOT_ALLOWED_AT_THIS_TIME,
  PASSWORD_EXPIRED,
  PASSWORD_CHANGE_REQUIRED = 100,
  SESSION_STATUS_OTHER = 101
};

enum TimeInForce {
  TIF_DAY = 0,
  TIF_IOC = 3,
  TIF_FOK = 4,
  TIF_AT_CROSSING = 9,
};

template <std::size_t N>
using AlphanumericFixedLength = char[N];

template <std::size_t N>
struct AlphanumericVariableLength {
  uint16_t len;
  char data[N];
};

using BrokerId = AlphanumericFixedLength<12>;
using BrokerLocationId = AlphanumericFixedLength<11>;
using BusinessRejectReferenceId = AlphanumericFixedLength<21>;
using Checksum = uint32_t;
using ClearingInstruction = uint8_t;
using CompId = AlphanumericFixedLength<12>;
using Decimal = uint64_t;
// A bit map with each bit representing a specific disclosure type:
//  * Bit 0 = None (NO specific information to disclose)
// (It's for future use. EP should specify bit 0 as 1 for completeness)
using DisclosureInstructions = uint16_t;
using EntitlementRequestId = AlphanumericFixedLength<21>;
using ExchangeTradeType = uint8_t;
using ExecRestatementReason = uint16_t;
using ExecType = int8_t;
using ExecutionId = AlphanumericFixedLength<21>;
// Multiple values can be sent separated by a space
// Instructions for order handling on exchange trading floor
//  * 0 = ingnore price validity checks
//  * 1 = ignore notional value checks
// Absence of this field if interpreted as None
//   (i.e. system will perform both price and notional value check)
using ExecutionInstructions = AlphanumericFixedLength<21>;
using ReferenceFieldName = AlphanumericFixedLength<50>;
using GapFill = uint8_t;  // 'N' = reset, 'Y' = gap fill
using IpType = AlphanumericFixedLength<16>;
using Length = uint16_t;
using LogoutText = AlphanumericVariableLength<75>;
using LotType = uint8_t;
using MarketSegmentId = AlphanumericFixedLength<20>;
using MassActionReportId = AlphanumericFixedLength<21>;
// 1 = cancel orders for security
// 7 = cancel all orders
// 9 = cancel orders for a market segment
using MassCancelRequestType = uint8_t;
using MassCancelResponse = uint8_t;
using MatchType = uint8_t;
using MaxPriceLevels = uint8_t;
using MessageSequence = uint32_t;
using MessageType = uint8_t;
using OrderCapacity = uint8_t;
using OrderCategory = uint8_t;
using OrderId = AlphanumericFixedLength<21>;
using OrderRestrictions = AlphanumericFixedLength<21>;
using OrderStatus = uint8_t;
using OrderType = uint8_t;  // 1: Market  2: Limit
using Password = AlphanumericFixedLength<450>;
using PortType = uint16_t;
using PositionEffect = uint8_t;  // 1: close Applicable if side = 1
using PossDupFlag = uint8_t;
using PossResendFlag = uint8_t;
using PresenceBitmap = uint8_t[32];
using Price = Decimal;
using ProtocolType = uint8_t;
using Quantity = Decimal;
using QuoteCancelType = uint8_t;
using QuoteId = AlphanumericFixedLength<21>;
using QuoteMessageId = AlphanumericFixedLength<21>;
using QuoteStatus = uint8_t;
using QuoteType = uint8_t;
using Reason = AlphanumericVariableLength<75>;
using RejectCode = uint16_t;
using SecurityExchange = AlphanumericFixedLength<5>;
using SecurityId = AlphanumericFixedLength<21>;
using SecurityIdSource = uint8_t;
// 0 = Session active
// 1 = Session password change
// 2 = Session password due to expire
// 3 = New session password does not comply with the policy
// 4 = Session logout complete
// 5 = Invalid username or password
// 6 = Account locked
// 7 = Logons are not allowed at this time
// 8 = Password expired
// 100 = Password change is required
// 101 = Other
using SessionStatus = uint8_t;
using Side = uint8_t;  // 1: Buy  2: Sell  5: Sell Short
using StartOfMessage = uint8_t;
using Status = uint8_t;
using TestMessageIndicator = uint8_t;
using TestRequestId = uint16_t;
using Text = AlphanumericVariableLength<50>;
using Tif = uint8_t;  // 0: Day  3: IOC  4: FOK  9: At Crossing
using TradeHandlingInstructions = uint8_t;
using TradeId = AlphanumericFixedLength<25>;
using TradeMatchId = AlphanumericFixedLength<25>;
using TradeReportId = AlphanumericFixedLength<21>;
using TradeReportStatus = uint8_t;
using TradeReportTransType = uint8_t;
using TradeReportType = uint8_t;
using TradeType = uint8_t;
using TypeOfService = uint8_t;
using TransactionTime = AlphanumericFixedLength<25>;  // YYYYMMDD-HH:MM:SS.sss

struct MessageHeader {
  StartOfMessage start_of_message;
  Length length;
  MessageType message_type;
  MessageSequence sequence_number;
  PossDupFlag poss_dup_flag;
  PossResendFlag poss_resend_flag;
  CompId comp_id;
  PresenceBitmap body_fields_presence_map;
} __attribute__((__packed__));

struct MessageTrailer {
  Checksum checksum;
} __attribute__((__packed__));

struct LookupRequest {
  TypeOfService type_of_service;
  ProtocolType protocol_type;
};

struct LookupResponse {
  Status status;  // 0: Y. 0 = Rejected, 1 = Accepted
  // 0 = Invalid Client
  // 1 = Invalid service type
  // 2 = Invalid Protocol
  // 3 = Client is blocked
  // 4 = Other
  RejectCode lookup_reject_code;  // 1: N
  Reason reason;                  // 2: N
  IpType primary_ip;              // 3: N
  PortType primary_port;          // 4: N
  IpType secondary_ip;            // 5: N
  PortType secondary_port;        // 6: N
};

struct LogonMessage {
  Password password;      // 0: N. As for BSS this field must be present
  Password new_password;  // 1: N
  MessageSequence next_expected_message_sequence;  // 2: Y
  SessionStatus session_status;                    // 3: N
  Text text;                                       // 4: N
  TestMessageIndicator test_message_indicator;     // 5: N
};

struct LogoutMessage {
  LogoutText logout_text;        // 0: N
  SessionStatus session_status;  // 1: N
};

struct HeartbeatMessage {
  TestRequestId reference_test_request_id;  // 0: N
};

struct TestRequest {
  TestRequestId test_request_id;  // 0: Y
};

struct ResendRequest {
  MessageSequence start_sequence;  // 0: Y
  MessageSequence end_sequence;    // 1: Y
};

struct RejectMessage {
  RejectCode message_reject_code;             // 0: Y
  Reason reason;                              // 1: N
  MessageType reference_message_type;         // 2: N
  ReferenceFieldName reference_field_name;    // 3: N
  MessageSequence reference_sequence_number;  // 4: Y
  OrderId client_order_id;                    // 5: N
};

struct SequenceResetMessage {
  GapFill gap_fill;                     // 0: N. As for BSS must be 'Y'
  MessageSequence new_sequence_number;  // 1: Y
};

struct PartyEntitlementRequest {
  EntitlementRequestId entitlement_request_id;  // Y
};

struct NewOrderRequest {
  OrderId client_order_id;              // 0: Y
  BrokerId submitting_broker_id;        // 1: Y
  SecurityId security_id;               // 2: Y
  SecurityIdSource security_id_source;  // 3: Y
  SecurityExchange security_exchange;   // 4: N
  BrokerLocationId broker_location_id;  // 5: N
  TransactionTime transaction_time;     // 6: Y
  Side side;             // 7: Y, 1 = buy, 2 = sell, 5 = sell short
  OrderType order_type;  // 8: Y. 1 = market, 2 = limit
  // 9: N. if price is 1.23, set this field to 123,000,000 (1.23 * 1e8)
  Price price;
  // 10: Y. if quantity is 100, set this field to 10,000,000,000 (100 * 1e8)
  Quantity order_quantity;
  Tif tif;  // 11: N. 0 = day, 3 = IOC, 4 = FOK, 9 = at crossing
  // 12: N. 1 = close
  // applicable only if side = 1(buy) to indicatecovering a short sell
  PositionEffect position_effect;
  OrderRestrictions order_restrictions;  // 13: N
  // 14: N. if present, this should be set as 1
  MaxPriceLevels max_price_levels;
  OrderCapacity order_capacity;  // 15: N. 1 = Agent, 2 = principal
  Text text;                     // 16: N
  ExecutionInstructions execution_instructions;    // 17: N
  DisclosureInstructions disclosure_instructions;  // 18: Y
  LotType lot_type;  // 19: N. not included = board lot, 1 = odd lot
};

struct CancelRequest {
  OrderId client_order_id;              // 0: Y. client order id
  BrokerId submitting_broker_id;        // 1: Y. submitting broker id
  SecurityId security_id;               // 2: Y.
  SecurityIdSource security_id_source;  // 3: Y.
  SecurityExchange security_exchange;   // 4: N.
  BrokerLocationId broker_location_id;  // 5: N.
  TransactionTime transaction_time;     // 6: Y.
  Side side;                            // 7: Y.
  OrderId original_client_order_id;     // 8: Y. original client order id
  OrderId order_id;                     // 9: N. order id
  Text text;                            // 10: N. free text
};

struct AmendRequest {
  OrderId client_order_id;                         // 0: Y
  BrokerId submitting_broker_id;                   // 1: Y
  SecurityId security_id;                          // 2: Y
  SecurityIdSource security_id_source;             // 3: Y
  SecurityExchange security_exchange;              // 4: N
  BrokerLocationId broker_location_id;             // 5: N
  TransactionTime transaction_time;                // 6: Y
  Side side;                                       // 7: Y
  OrderId original_client_order_id;                // 8: Y
  OrderId order_id;                                // 9: N
  OrderType order_type;                            // 10: Y
  Price price;                                     // 11: N
  Quantity order_quantity;                         // 12: N
  Tif tif;                                         // 13: N
  PositionEffect position_effect;                  // 14: N
  OrderRestrictions order_restrictions;            // 15: N
  MaxPriceLevels max_price_levels;                 // 16: N
  OrderCapacity order_capacity;                    // 17: N
  Text text;                                       // 18: N
  ExecutionInstructions execution_instructions;    // 19: N
  DisclosureInstructions disclosure_instructions;  // 20: Y
};

struct MassCancelRequest {
  OrderId client_order_id;              // 0: Y. client order id
  BrokerId submitting_broker_id;        // 1: Y. submitting broker id
  SecurityId security_id;               // 2: N.
  SecurityIdSource security_id_source;  // 3: N.
  SecurityExchange security_exchange;   // 4: N.
  BrokerLocationId broker_location_id;  // 5: N.
  TransactionTime transaction_time;     // 6: Y.
  Side side;                            // 7: N. 1 = buy, 2 = sell
  // 8: Y
  // 1 = cancel orders for a security
  // 7 = cancel all orders
  // 9 = cancel orders for a market segment
  MassCancelRequestType mass_cancel_request_type;
  // 9: N
  // identify the market segment: MAIN, GEM, NASD, ETS
  // required if mass_cancel_request_type = 9
  MarketSegmentId market_segment_id;
};

struct OboCancelRequest {
  OrderId client_order_id;              // 0: Y
  BrokerId submitting_broker_id;        // 1: Y
  SecurityId security_id;               // 2: Y
  SecurityIdSource security_id_source;  // 3: Y
  SecurityExchange security_exchange;   // 4: N
  BrokerLocationId broker_location_id;  // 5: N
  TransactionTime transaction_time;     // 6: Y
  Side side;                         // 7: Y. 1 = buy, 2 = sell, 5 = sell short
  OrderId original_client_order_id;  // 8: N
  OrderId order_id;                  // 9: Y
  BrokerId owning_broker_id;         // 10: Y
  Text text;                         // 11: N. free text
};

struct OboMassCancelRequest {
  OrderId client_order_id;              // 0: Y
  BrokerId submitting_broker_id;        // 1: Y
  SecurityId security_id;               // 2: N
  SecurityIdSource security_id_source;  // 3: N
  SecurityExchange security_exchange;   // 4: N
  BrokerLocationId broker_location_id;  // 5: N
  TransactionTime transaction_time;     // 6: Y
  Side side;  // 7: N. 1 = buy, 2 = sell, 5 = sell short
  MassCancelRequestType mass_cancel_request_type;  // 8: Y
  MarketSegmentId market_segment_id;               // 9: N
  BrokerId owning_broker_id;                       // 10: Y
};

// The OCG will send this execution report once the new order
// (board or odd lot/special lot) is accepted
struct OrderAcceptedReport {
  OrderId client_order_id;               // 0: Y
  BrokerId submitting_broker_id;         // 1: Y
  SecurityId security_id;                // 2: Y
  SecurityIdSource security_id_source;   // 3: Y
  SecurityExchange security_exchange;    // 4: N
  BrokerLocationId broker_location_id;   // 5: N
  TransactionTime transaction_time;      // 6: Y
  Side side;                             // 7: Y
  OrderId order_id;                      // 9: Y
  OrderType order_type;                  // 11: N
  Price price;                           // 12: N
  Quantity order_quantity;               // 13: N
  Tif tif;                               // 14: N
  PositionEffect position_effect;        // 15: N
  OrderRestrictions order_restrictions;  // 16: N
  MaxPriceLevels max_price_levels;       // 17: N
  OrderCapacity order_capacity;          // 18: N
  Text text;                             // 19: N
  ExecutionId execution_id;              // 21: Y
  OrderStatus order_status;              // 22: Y
  ExecType exec_type;                    // 23: Y
  Quantity cumulative_quantity;          // 24: Y
  Quantity leaves_quantity;              // 25: Y
  LotType lot_type;                      // 27: N. 1 = odd lot, 2 = round lot
};

// The OCG will send this execution report once the new order
// (board or odd lot/special lot) is rejected
struct OrderRejectedReport {
  OrderId client_order_id;               // 0: Y
  BrokerId submitting_broker_id;         // 1: Y
  SecurityId security_id;                // 2: Y
  SecurityIdSource security_id_source;   // 3: Y
  SecurityExchange security_exchange;    // 4: N
  BrokerLocationId broker_location_id;   // 5: N
  TransactionTime transaction_time;      // 6: Y
  Side side;                             // 7: Y
  OrderId order_id;                      // 9: Y
  OrderType order_type;                  // 11: N
  Price price;                           // 12: N
  Quantity order_quantity;               // 13: N
  Tif tif;                               // 14: N
  PositionEffect position_effect;        // 15: N
  OrderRestrictions order_restrictions;  // 16: N
  MaxPriceLevels max_price_levels;       // 17: N
  OrderCapacity order_capacity;          // 18: N
  Text text;                             // 19: N
  Reason reason;                         // 20: N
  ExecutionId execution_id;              // 21: Y
  OrderStatus order_status;              // 22: Y
  ExecType exec_type;                    // 23: Y
  Quantity cumulative_quantity;          // 24: Y
  Quantity leaves_quantity;              // 25: Y
  // 26: N
  // 3 = order exceed limit
  // 6 = duplicate order
  // 13 = incorrect quantity
  // 16 = price exceeds current price band
  // 19 = reference price is not available
  // 20 = notional value exceeds threshold
  // 99 = other
  // 101 = price exceeds current price band (override not allowed)
  // 102 = price exceeds current price band
  RejectCode order_reject_code;
};

// The OCG will send this execution report once the Cancel Request
// for an order (board or odd lot/special lot) is accepted
struct OrderCancelledReport {
  OrderId client_order_id;               // 0: Y
  BrokerId submitting_broker_id;         // 1: Y
  SecurityId security_id;                // 2: Y
  SecurityIdSource security_id_source;   // 3: Y
  SecurityExchange security_exchange;    // 4: N
  BrokerLocationId broker_location_id;   // 5: N
  TransactionTime transaction_time;      // 6: Y
  Side side;                             // 7: Y
  OrderId original_client_order_id;      // 8: Y
  OrderId order_id;                      // 9: Y
  OrderType order_type;                  // 11: N
  Price price;                           // 12: N
  Quantity order_quantity;               // 13: N
  Tif tif;                               // 14: N
  PositionEffect position_effect;        // 15: N
  OrderRestrictions order_restrictions;  // 16: N
  MaxPriceLevels max_price_levels;       // 17: N
  OrderCapacity order_capacity;          // 18: N
  Text text;                             // 19: N
  ExecutionId execution_id;              // 21: Y
  OrderStatus order_status;              // 22: Y
  ExecType exec_type;                    // 23: Y
  Quantity cumulative_quantity;          // 24: Y
  Quantity leaves_quantity;              // 25: Y
};

// The OCG will send this execution report once the Cancel Request
// for an order (board or odd lot/special lot) is accepted
struct UnsolicitedOrderCanceledReport {
  OrderId client_order_id;               // 0: Y
  BrokerId submitting_broker_id;         // 1: Y
  SecurityId security_id;                // 2: Y
  SecurityIdSource security_id_source;   // 3: Y
  SecurityExchange security_exchange;    // 4: N
  BrokerLocationId broker_location_id;   // 5: N
  TransactionTime transaction_time;      // 6: Y
  Side side;                             // 7: Y
  OrderId order_id;                      // 9: Y
  OrderType order_type;                  // 11: N
  Price price;                           // 12: N
  Quantity order_quantity;               // 13: N
  Tif tif;                               // 14: N
  PositionEffect position_effect;        // 15: N
  OrderRestrictions order_restrictions;  // 16: N
  MaxPriceLevels max_price_levels;       // 17: N
  OrderCapacity order_capacity;          // 18: N
  Text text;                             // 19: N
  ExecutionId execution_id;              // 21: Y
  OrderStatus order_status;              // 22: Y
  ExecType exec_type;                    // 23: Y
  Quantity cumulative_quantity;          // 24: Y
  Quantity leaves_quantity;              // 25: Y
  // 28: N
  // 6 = cancel on trading Halt/VCM
  // 8 = market operation
  // 100 = unsolicited cancel for original order (for cancel/replace
  //       operation which fails market validation)
  // 103 = mass canceled by broker
  // 104 = cancel on disconnect
  // 105 = cancel due to broker suspended
  // 106 = cancel due to exchange participant suspended
  // 107 = system cancel
  ExecRestatementReason exec_restatement_reason;
};

// The OCG will send this execution report for an OBO cancellation
// of an order (board or odd lot/special lot)
struct OboOrderCanceledReport {
  OrderId client_order_id;               // 0: Y
  BrokerId submitting_broker_id;         // 1: Y
  SecurityId security_id;                // 2: Y
  SecurityIdSource security_id_source;   // 3: Y
  SecurityExchange security_exchange;    // 4: N
  BrokerLocationId broker_location_id;   // 5: N
  TransactionTime transaction_time;      // 6: Y
  Side side;                             // 7: Y
  OrderId order_id;                      // 9: Y
  OrderType order_type;                  // 11: N
  Price price;                           // 12: N
  Quantity order_quantity;               // 13: N
  Tif tif;                               // 14: N
  PositionEffect position_effect;        // 15: N
  OrderRestrictions order_restrictions;  // 16: N
  MaxPriceLevels max_price_levels;       // 17: N
  OrderCapacity order_capacity;          // 18: N
  Text text;                             // 19: N
  ExecutionId execution_id;              // 21: Y
  OrderStatus order_status;              // 22: Y
  ExecType exec_type;                    // 23: Y
  Quantity cumulative_quantity;          // 24: Y
  Quantity leaves_quantity;              // 25: Y
  // 28: N. code to identify the reason for an execution report
  //        message with Exec Type = 4(Cancel)
  // 101 = on behalf of single cancel
  // 102 = on behalf of mass cancel
  ExecRestatementReason exec_restatement_reason;
};

// The OCG will send this execution report when an
// order (board lot or odd lot/special lot) expires
struct OrderExpiredReport {
  OrderId client_order_id;               // 0: Y
  BrokerId submitting_broker_id;         // 1: Y
  SecurityId security_id;                // 2: Y
  SecurityIdSource security_id_source;   // 3: Y
  SecurityExchange security_exchange;    // 4: N
  BrokerLocationId broker_location_id;   // 5: N
  TransactionTime transaction_time;      // 6: Y
  Side side;                             // 7: Y
  OrderId order_id;                      // 9: Y
  OrderType order_type;                  // 11: N
  Price price;                           // 12: N
  Quantity order_quantity;               // 13: N
  Tif tif;                               // 14: N
  PositionEffect position_effect;        // 15: N
  OrderRestrictions order_restrictions;  // 16: N
  MaxPriceLevels max_price_levels;       // 17: N
  OrderCapacity order_capacity;          // 18: N
  Text text;                             // 19: N
  Reason reason;                         // 20: N
  ExecutionId execution_id;              // 21: Y
  OrderStatus order_status;              // 22: Y
  ExecType exec_type;                    // 23: Y
  Quantity cumulative_quantity;          // 24: Y
  Quantity leaves_quantity;              // 25: Y
};

// The OCG will send this execution report when an Order Amend request
// for an order (board lot or odd lot/special lot) is accepted
struct OrderAmendedReport {
  OrderId client_order_id;               // 0: Y
  BrokerId submitting_broker_id;         // 1: Y
  SecurityId security_id;                // 2: Y
  SecurityIdSource security_id_source;   // 3: Y
  SecurityExchange security_exchange;    // 4: N
  BrokerLocationId broker_location_id;   // 5: N
  TransactionTime transaction_time;      // 6: Y
  Side side;                             // 7: Y
  OrderId original_client_order_id;      // 8: Y
  OrderId order_id;                      // 9: Y
  OrderType order_type;                  // 11: N
  Price price;                           // 12: N
  Quantity order_quantity;               // 13: N
  Tif tif;                               // 14: N
  PositionEffect position_effect;        // 15: N
  OrderRestrictions order_restrictions;  // 16: N
  MaxPriceLevels max_price_levels;       // 17: N
  OrderCapacity order_capacity;          // 18: N
  Text text;                             // 19: N
  ExecutionId execution_id;              // 21: Y
  OrderStatus order_status;              // 22: Y. 0 = new, 1 = partially filled
  ExecType exec_type;                    // 23: Y. 5 = amend
  Quantity cumulative_quantity;          // 24: Y
  Quantity leaves_quantity;              // 25: Y
};

// The OCG will send this execution report when an Order Cancel Request is
// rejected Note: Order Status is set to 8 = Rejected in the event of one of the
// following scenarios:
//      * It's an unknown order
//      * Validation failure of an OBO cancel request
struct OrderCancelRejectedReport {
  OrderId client_order_id;               // 0: Y
  BrokerId submitting_broker_id;         // 1: Y
  SecurityId security_id;                // 2: Y
  SecurityIdSource security_id_source;   // 3: Y
  SecurityExchange security_exchange;    // 4: N
  BrokerLocationId broker_location_id;   // 5: N
  TransactionTime transaction_time;      // 6: Y
  Side side;                             // 7: Y
  OrderId original_client_order_id;      // 8: N
  OrderId order_id;                      // 9: Y
  BrokerId owning_broker_id;             // 10: N
  OrderType order_type;                  // 11: N
  Price price;                           // 12: N
  Quantity order_quantity;               // 13: N
  Tif tif;                               // 14: N
  PositionEffect position_effect;        // 15: N
  OrderRestrictions order_restrictions;  // 16: N
  MaxPriceLevels max_price_levels;       // 17: N
  OrderCapacity order_capacity;          // 18: N
  Text text;                             // 19: N
  Reason reason;                         // 20: N
  ExecutionId execution_id;              // 21: Y
  // 22: Y
  // 0 = new
  // 1 = partially filled
  // 2 = filled
  // 4 = cancelled
  // 6 = pending cancel
  // 8 = rejected
  // 10 = pending new
  // 12 = expired
  // 14 = pending amend
  OrderStatus order_status;
  ExecType exec_type;            // 23: Y. X = cancel reject
  Quantity cumulative_quantity;  // 24: Y
  Quantity leaves_quantity;      // 25: Y
  // 29: N
  // 0 = too late to cancel
  // 1 = unknown order
  // 3 = order already in pending cancel or pending replaced status
  // 6 = duplicate client order id received
  // 99 = other (default)
  RejectCode cancel_reject_code;
};

// The OCG sends this execution report when an Order Amend Request is rejected
struct OrderAmendRejectedReport {
  OrderId client_order_id;              // 0: Y
  BrokerId submitting_broker_id;        // 1: Y
  SecurityId security_id;               // 2: Y
  SecurityIdSource security_id_source;  // 3: Y
  SecurityExchange security_exchange;   // 4: N
  BrokerLocationId broker_location_id;  // 5: N
  TransactionTime transaction_time;     // 6: Y
  Side side;                            // 7: Y
  OrderId original_client_order_id;     // 8: Y
  OrderId order_id;                     // 9: Y
  OrderType order_type;                 // 11: N
  Price price;                     // 12: N. required if Order Type = 2 (limit)
  Quantity order_quantity;         // 13: N
  Tif tif;                         // 14: N
  PositionEffect position_effect;  // 15: N
  OrderRestrictions order_restrictions;  // 16: N
  MaxPriceLevels max_price_levels;       // 17: N
  OrderCapacity order_capacity;          // 18: N
  Text text;                             // 19: N
  Reason reason;                         // 20: N
  ExecutionId execution_id;              // 21: Y
  // 22: Y
  // 0 = new
  // 1 = partially filled
  // 2 = filled
  // 4 = cancelled
  // 6 = pending cancel
  // 8 = rejected
  // 10 = pending new
  // 12 = expired
  // 14 = pending amend
  OrderStatus order_status;
  ExecType exec_type;            // 23: Y. Y = amend reject
  Quantity cumulative_quantity;  // 24: Y
  Quantity leaves_quantity;      // 25: Y
  // 36: N
  // 0 = too late to amend
  // 1 = unknown order
  // 3 = order already in pending cancel or pending replace status
  // 6 = duplicate client order id received
  // 8 = price exceeds current price band;
  // 99 = other
  // 100 = reference price is not available
  // 101 = price exceeds current price band (override not allowed)
  // 102 = price exceeds current price band
  // 103 = notional value exceeds threshold
  RejectCode amend_reject_code;
};

// The OCG sends this execution report for an auto-matched trade
struct TradeReport {
  OrderId client_order_id;              // 0: Y
  BrokerId submitting_broker_id;        // 1: Y
  SecurityId security_id;               // 2: Y
  SecurityIdSource security_id_source;  // 3: Y
  SecurityExchange security_exchange;   // 4: N
  BrokerLocationId broker_location_id;  // 5: N
  TransactionTime transaction_time;     // 6: Y
  Side side;                            // 7: Y
  OrderId order_id;                     // 9: Y
  OrderType order_type;                 // 11: N
  Price price;                     // 12: N. required if Order Type = 2 (limit)
  Quantity order_quantity;         // 13: N
  Tif tif;                         // 14: N
  PositionEffect position_effect;  // 15: N
  OrderRestrictions order_restrictions;  // 16: N
  MaxPriceLevels max_price_levels;       // 17: N
  OrderCapacity order_capacity;          // 18: N
  Text text;                             // 19: N
  ExecutionId execution_id;              // 21: Y
  // 22: Y
  // 1 = partially filled
  // 2 = filled
  OrderStatus order_status;
  ExecType exec_type;            // 23: Y. F = trade
  Quantity cumulative_quantity;  // 24: Y
  Quantity leaves_quantity;      // 25: Y
  // 27: N
  // 2 = round lot
  // Absence of this field indicates a round lot order
  LotType lot_type;
  // 30: N
  // 4 = auto match
  // 5 = cross auction
  MatchType match_type;
  BrokerId counterparty_broker_id;  // 31: N
  Quantity execution_quantity;      // 32: Y
  Price execution_price;            // 33: Y
  // 35: N
  // 1 = internal cross order
  // absence of this field means the trade is not concluded within the same firm
  OrderCategory order_category;
  TradeMatchId trade_match_id;  // 38: N
};

// This execution report message is sent by the OCG when an
// auto-matched trade is cancelled by the exchange
struct AutoMatchedTradeCancelledReport {
  OrderId client_order_id;              // 0: Y
  BrokerId submitting_broker_id;        // 1: Y
  SecurityId security_id;               // 2: Y
  SecurityIdSource security_id_source;  // 3: Y
  SecurityExchange security_exchange;   // 4: N
  BrokerLocationId broker_location_id;  // 5: N
  TransactionTime transaction_time;     // 6: Y
  Side side;                            // 7: Y
  OrderId order_id;                     // 9: Y
  OrderType order_type;                 // 11: N
  Price price;                     // 12: N. required if Order Type = 2 (limit)
  Quantity order_quantity;         // 13: N
  Tif tif;                         // 14: N
  PositionEffect position_effect;  // 15: N
  OrderRestrictions order_restrictions;  // 16: N
  MaxPriceLevels max_price_levels;       // 17: N
  OrderCapacity order_capacity;          // 18: N
  Text text;                             // 19: N
  ExecutionId execution_id;              // 21: Y
  // 22: Y
  // 0 = new
  // 1 = partially filled
  // 2 = filled
  // 4 = cancelled
  // 12 = expired
  OrderStatus order_status;
  ExecType exec_type;                         // 23: Y. H = trade cancel
  Quantity cumulative_quantity;               // 24: Y
  Quantity leaves_quantity;                   // 25: Y
  ExecRestatementReason exec_restate_reason;  // 28: N. 8 = market operations
  Quantity execution_quantity;                // 32: N
  Price execution_price;                      // 33: N
  ExecutionId reference_execution_id;         // 34: Y. execution ID previously
                                              // published for this trade
  // 35: N
  // 1 = internal cross order
  // absence of this field means the trade is not concluded within the same firm
  OrderCategory order_category;
};

// The OCG sends this execution report for an odd lot/special lot order when it
// is filled
struct OddSpecLotOrderTradeReport {
  OrderId client_order_id;              // 0: Y
  BrokerId submitting_broker_id;        // 1: Y
  SecurityId security_id;               // 2: Y
  SecurityIdSource security_id_source;  // 3: Y
  SecurityExchange security_exchange;   // 4: N
  BrokerLocationId broker_location_id;  // 5: N
  TransactionTime transaction_time;     // 6: Y
  Side side;                            // 7: Y
  OrderId order_id;                     // 9: Y
  OrderType order_type;                 // 11: N
  Price price;                     // 12: N. required if Order Type = 2 (limit)
  Quantity order_quantity;         // 13: N
  Tif tif;                         // 14: N
  PositionEffect position_effect;  // 15: N
  OrderCapacity order_capacity;    // 18: N
  Text text;                       // 19: N
  ExecutionId execution_id;        // 21: Y
  // 22: Y
  // 2 = filled
  OrderStatus order_status;
  ExecType exec_type;            // 23: Y. F = trade
  Quantity cumulative_quantity;  // 24: Y
  Quantity leaves_quantity;      // 25: Y
  // 27: N
  // 1 = odd lot
  // Absence of this field indicates a round lot order
  LotType lot_type;
  BrokerId counterparty_broker_id;  // 31: N
  Quantity execution_quantity;      // 32: Y
  Price execution_price;            // 33: Y
  // 35: N
  // 1 = internal cross order
  // absence of this field means the trade is not concluded within the same firm
  OrderCategory order_category;
  TradeMatchId trade_match_id;  // 38: N
  // 39: N
  // E = special lot -- semi-automatic
  // O = odd lot -- semi-automatic
  ExchangeTradeType exchange_trade_type;
};

// The OCG sends this execution report when a semi-auto-matched trade is
// cancelled by the exchange. Note that this trade cancellation message is sent
// to the Side that refers to an Odd lot/Special lot order
struct SemiAutoMatchedTradeCancelledReport {
  OrderId client_order_id;              // 0: Y
  BrokerId submitting_broker_id;        // 1: Y
  SecurityId security_id;               // 2: Y
  SecurityIdSource security_id_source;  // 3: Y
  SecurityExchange security_exchange;   // 4: N
  BrokerLocationId broker_location_id;  // 5: N
  TransactionTime transaction_time;     // 6: Y
  Side side;                            // 7: Y
  OrderId order_id;                     // 9: Y
  OrderType order_type;                 // 11: N
  Price price;                     // 12: N. required if Order Type = 2 (limit)
  Quantity order_quantity;         // 13: N
  Tif tif;                         // 14: N
  PositionEffect position_effect;  // 15: N
  OrderCapacity order_capacity;    // 18: N
  Text text;                       // 19: N
  ExecutionId execution_id;        // 21: Y
  // 22: Y
  // 0 = new
  // 1 = partially filled
  // 2 = filled
  // 4 = cancelled
  // 12 = expired
  OrderStatus order_status;
  ExecType exec_type;            // 23: Y. H = trade
  Quantity cumulative_quantity;  // 24: Y
  Quantity leaves_quantity;      // 25: Y
  ExecRestatementReason
      exec_restatement_reason;         // 28: N. 8 = market operations
  Quantity execution_quantity;         // 32: N
  Price execution_price;               // 33: N
  ExecutionId reference_execution_id;  // 34: Y
  // 35: N
  // 1 = internal cross order
  // absence of this field means the trade is not concluded within the same firm
  OrderCategory order_category;
};

struct ExecutionReport {
  OrderId client_order_id;                        // 0: Y
  BrokerId submitting_broker_id;                  // 1: Y
  SecurityId security_id;                         // 2: Y
  SecurityIdSource security_id_source;            // 3: Y
  SecurityExchange security_exchange;             // 4: N
  BrokerLocationId broker_location_id;            // 5: N
  TransactionTime transaction_time;               // 6: Y
  Side side;                                      // 7: Y
  OrderId original_client_order_id;               // 8: N
  OrderId order_id;                               // 9: Y
  BrokerId owning_broker_id;                      // 10: N
  OrderType order_type;                           // 11: N
  Price price;                                    // 12: N
  Quantity order_quantity;                        // 13: N
  Tif tif;                                        // 14: N
  PositionEffect position_effect;                 // 15: N
  OrderRestrictions order_restrictions;           // 16: N
  MaxPriceLevels max_price_levels;                // 17: N
  OrderCapacity order_capacity;                   // 18: N
  Text text;                                      // 19: N
  Reason reason;                                  // 20: N
  ExecutionId execution_id;                       // 21: Y
  OrderStatus order_status;                       // 22: Y
  ExecType exec_type;                             // 23: Y
  Quantity cumulative_quantity;                   // 24: Y
  Quantity leaves_quantity;                       // 25: Y
  RejectCode order_reject_code;                   // 26: N
  LotType lot_type;                               // 27: N
  ExecRestatementReason exec_restatement_reason;  // 28: N
  RejectCode cancel_reject_code;                  // 29: N
  MatchType match_type;                           // 30: N
  BrokerId counterparty_broker_id;                // 31: N
  Quantity execution_quantity;                    // 32: N
  Price execution_price;                          // 33: N
  ExecutionId reference_execution_id;             // 34: N
  OrderCategory order_category;                   // 35: N
  RejectCode amend_reject_code;                   // 36: N
  TradeMatchId trade_match_id;                    // 38: N
  ExchangeTradeType exchange_trade_type;          // 39: N
};

// The OCG sends this message in response to an Order Mass Cancel Request
// message
struct OrderMassCancelReport {
  OrderId client_order_id;                         // 0: N
  BrokerId submitting_broker_id;                   // 1: N
  SecurityId security_id;                          // 2: N
  SecurityIdSource security_id_source;             // 3: N
  SecurityExchange security_exchange;              // 4: N
  BrokerLocationId broker_location_id;             // 5: N
  TransactionTime transaction_time;                // 6: Y
  MassCancelRequestType mass_cancel_request_type;  // 7: Y
  BrokerId owning_broker_id;                       // 8: N
  MassActionReportId mass_action_report_id;        // 9: Y
  // 10: Y
  // 0 = cancel request rejected
  // 1 = cancel order for a security
  // 7 = cancel all orders
  // 9 = cancel all orders for a market segment
  MassCancelResponse mass_cancel_response;
  // 11: N
  // 8 = invalid or unknown market segment
  // 99 = other
  // required if Mass Cancel Response = Cancel Request Rejected
  RejectCode mass_cancel_reject_code;
  Reason reason;  // 12: N
};

struct TradeCaptureReport {
  TradeReportId trade_report_id;                          // 0: N
  TradeReportTransType trade_report_trans_type;           // 1: N
  TradeReportType trade_report_type;                      // 2: Y
  TradeHandlingInstructions trade_handling_instructions;  // 3: N
  BrokerId submitting_broker_id;                          // 4: Y
  BrokerId counterparty_broker_id;                        // 5: N
  BrokerLocationId broker_location_id;                    // 6: N
  SecurityId security_id;                                 // 7: Y
  SecurityIdSource security_id_source;                    // 8: Y
  SecurityExchange security_exchange;                     // 9: N
  Side side;                                              // 10: Y
  TransactionTime transaction_time;                       // 11: Y
  TradeId trade_id;                                       // 12: N
  TradeType trade_type;                                   // 13: N
  Quantity execution_quantity;                            // 14: Y
  Price execution_price;                                  // 15: Y
  ClearingInstruction clearing_instruction;               // 16: N
  PositionEffect position_effect;                         // 17: N
  OrderCapacity order_capacity;                           // 19: N
  OrderCategory order_category;                           // 20: N
  Text text;                                              // 21: N
  ExecutionInstructions execution_instructions;           // 22: N
  ExecType exec_type;                                     // 23: N
  TradeReportStatus trade_report_status;                  // 24: N
  ExchangeTradeType exchange_trade_type;                  // 25: N
  OrderId order_id;                                       // 27: N
};

struct TradeCaptureReportAck {
  TradeReportId trade_report_id;                          // 0: Y
  TradeReportTransType trade_report_trans_type;           // 1: N
  TradeReportType trade_report_type;                      // 2: Y
  TradeHandlingInstructions trade_handling_instructions;  // 3: N
  BrokerId submitting_broker_id;                          // 4: Y
  BrokerId counterparty_broker_id;                        // 5: N
  BrokerLocationId broker_location_id;                    // 6: N
  SecurityId security_id;                                 // 7: Y
  SecurityIdSource security_id_source;                    // 8: Y
  SecurityExchange security_exchange;                     // 9: N
  Side side;                                              // 10: Y
  TransactionTime transaction_time;                       // 11: Y
  TradeId trade_id;                                       // 12: N
  TradeReportStatus trade_report_status;                  // 13: N
  RejectCode trade_report_reject_code;                    // 14: N
  Reason reason;                                          // 15: N
};

struct BusinessRejectMessage {
  RejectCode business_reject_code;                         // 0: Y
  Reason reason;                                           // 1: N
  MessageType reference_message_type;                      // 2: Y
  ReferenceFieldName reference_field_name;                 // 3: N
  MessageSequence reference_sequence_number;               // 4: N
  BusinessRejectReferenceId business_reject_reference_id;  // 5: N
};

struct QuoteRequest {
  BrokerId submitting_broker_id;                 // 0: Y
  BrokerLocationId broker_location_id;           // 1: N
  SecurityId security_id;                        // 2: Y
  SecurityIdSource security_id_source;           // 3: Y
  SecurityExchange security_exchange;            // 4: N
  QuoteId quote_bid_id;                          // 5: Y
  QuoteId quote_offer_id;                        // 6: Y
  QuoteType quote_type;                          // 7: N
  Side side;                                     // 8: N
  Quantity bid_size;                             // 9: Y
  Quantity offer_size;                           // 10: Y
  Price bid_price;                               // 11: Y
  Price offer_price;                             // 12: Y
  TransactionTime transaction_time;              // 13: Y
  PositionEffect position_effect;                // 14: N
  OrderRestrictions order_restrictions;          // 15: N
  Text text;                                     // 16: N
  ExecutionInstructions execution_instructions;  // 17: N
};

struct QuoteCancelRequest {
  BrokerId submitting_broker_id;        // 0: Y
  BrokerLocationId broker_location_id;  // 1: N
  SecurityId security_id;               // 2: N
  SecurityIdSource security_id_source;  // 3: N
  SecurityExchange security_exchange;   // 4: N
  QuoteMessageId quote_message_id;      // 5: Y
  QuoteCancelType quote_cancel_type;    // 6: Y
};

struct QuoteStatusReport {
  BrokerId submitting_broker_id;        // 0: Y
  BrokerLocationId broker_location_id;  // 1: N
  SecurityId security_id;               // 2: N
  SecurityIdSource security_id_source;  // 3: N
  SecurityExchange security_exchange;   // 4: N
  QuoteId quote_bid_id;                 // 5: N
  QuoteId quote_offer_id;               // 6: N
  QuoteType quote_type;                 // 7: N
  TransactionTime transaction_time;     // 8: Y
  QuoteMessageId quote_message_id;      // 9: N
  QuoteCancelType quote_cancel_type;    // 10: N
  QuoteStatus quote_status;             // 11: N
  RejectCode quote_reject_code;         // 12: N
  Reason reason;                        // 13: N
};

// struct PartyEntitlementReport {
//  EntitlementReportId entitlement_report_id;
//  EntitlementRequestId entitlement_request_id;
//  RequestResult request_result;
//  TotalNoPartyList total_no_party_list;
//  LastFragment last_fragment;
//  BrokerId broker_id;
//};

}  // namespace ft::bss

#endif  // OCG_BSS_PROTOCOL_H_
