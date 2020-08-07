// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef OCG_BSS_TESTCASE_H_
#define OCG_BSS_TESTCASE_H_

#include "protocol/protocol.h"

using namespace ft::bss;

inline LogonMessage logon_msg = {
    "aaaa",   // password
    "",       // new_password
    1,        // next_expected_message_sequence
    0,        // session_status
    {0, ""},  // text
    0         // test_message_indicator
};

inline NewOrderRequest tc_om_1 = {
    "10",                  // client_order_id
    "1122",                // submitting_broker_id
    "36",                  // security_id
    8,                     // security_id_source
    "XHKG",                // security_exchange
    "unused",              // broker_location_id
    "unused",              // transaction_time
    1,                     // side
    2,                     // order_type
    uint64_t(30 * 1e8),    // price
    uint64_t(4000 * 1e8),  // order_quantity
    0,                     // tif
    0,                     // position_effect
    "",                    // order_restrictions
    1,                     // max_price_levels
    0,                     // order_capacity
    {0, ""},               // text
    "0 1",                 // execution_instructions
    0x0001,                // disclosure_instructions
    0                      // lot_type
};

inline NewOrderRequest tc_om_2 = {
    "11",                   // client_order_id
    "1122",                 // submitting_broker_id
    "36",                   // security_id
    8,                      // security_id_source
    "XHKG",                 // security_exchange
    "unused",               // broker_location_id
    "unused",               // transaction_time
    1,                      // side
    2,                      // order_type
    uint64_t(29.95 * 1e8),  // price
    uint64_t(4000 * 1e8),   // order_quantity
    0,                      // tif
    0,                      // position_effect
    "",                     // order_restrictions
    0,                      // max_price_levels
    0,                      // order_capacity
    {0, ""},                // text
    "0 1",                  // execution_instructions
    0x0001,                 // disclosure_instructions
    0                       // lot_type
};

inline NewOrderRequest tc_om_3 = {
    "12",                  // client_order_id
    "1122",                // submitting_broker_id
    "36",                  // security_id
    8,                     // security_id_source
    "XHKG",                // security_exchange
    "unused",              // broker_location_id
    "unused",              // transaction_time
    1,                     // side
    2,                     // order_type
    uint64_t(29.9 * 1e8),  // price
    uint64_t(4000 * 1e8),  // order_quantity
    3,                     // tif
    0,                     // position_effect
    "",                    // order_restrictions
    0,                     // max_price_levels
    0,                     // order_capacity
    {0, ""},               // text
    "0 1",                 // execution_instructions
    0x0001,                // disclosure_instructions
    0                      // lot_type
};

inline NewOrderRequest tc_om_4 = {
    "13",                   // client_order_id
    "1122",                 // submitting_broker_id
    "36",                   // security_id
    8,                      // security_id_source
    "XHKG",                 // security_exchange
    "unused",               // broker_location_id
    "unused",               // transaction_time
    2,                      // side
    2,                      // order_type
    uint64_t(30.05 * 1e8),  // price
    uint64_t(4000 * 1e8),   // order_quantity
    0,                      // tif
    0,                      // position_effect
    "",                     // order_restrictions
    1,                      // max_price_levels
    0,                      // order_capacity
    {0, ""},                // text
    "0 1",                  // execution_instructions
    0x0001,                 // disclosure_instructions
    0                       // lot_type
};

inline NewOrderRequest tc_om_5 = {
    "14",                  // client_order_id
    "1122",                // submitting_broker_id
    "36",                  // security_id
    8,                     // security_id_source
    "XHKG",                // security_exchange
    "unused",              // broker_location_id
    "unused",              // transaction_time
    2,                     // side
    2,                     // order_type
    uint64_t(30.1 * 1e8),  // price
    uint64_t(4000 * 1e8),  // order_quantity
    0,                     // tif
    0,                     // position_effect
    "",                    // order_restrictions
    0,                     // max_price_levels
    0,                     // order_capacity
    {0, ""},               // text
    "0 1",                 // execution_instructions
    0x0001,                // disclosure_instructions
    0                      // lot_type
};

inline NewOrderRequest tc_om_6 = {
    "15",                  // client_order_id
    "1122",                // submitting_broker_id
    "36",                  // security_id
    8,                     // security_id_source
    "XHKG",                // security_exchange
    "unused",              // broker_location_id
    "unused",              // transaction_time
    2,                     // side
    1,                     // order_type
    0,                     // price
    uint64_t(4000 * 1e8),  // order_quantity
    9,                     // tif
    0,                     // position_effect
    "",                    // order_restrictions
    0,                     // max_price_levels
    0,                     // order_capacity
    {0, ""},               // text
    "0 1",                 // execution_instructions
    0x0001,                // disclosure_instructions
    0                      // lot_type
};

inline NewOrderRequest tc_om_7 = {
    "16",                   // client_order_id
    "1122",                 // submitting_broker_id
    "36",                   // security_id
    8,                      // security_id_source
    "XHKG",                 // security_exchange
    "unused",               // broker_location_id
    "unused",               // transaction_time
    2,                      // side
    2,                      // order_type
    uint64_t(30.25 * 1e8),  // price
    uint64_t(4000 * 1e8),   // order_quantity
    9,                      // tif
    0,                      // position_effect
    "",                     // order_restrictions
    0,                      // max_price_levels
    0,                      // order_capacity
    {0, ""},                // text
    "0 1",                  // execution_instructions
    0x0001,                 // disclosure_instructions
    0                       // lot_type
};

inline NewOrderRequest tc_om_8 = {
    "17",                   // client_order_id
    "1123",                 // submitting_broker_id
    "36",                   // security_id
    8,                      // security_id_source
    "XHKG",                 // security_exchange
    "unused",               // broker_location_id
    "unused",               // transaction_time
    1,                      // side
    2,                      // order_type
    uint64_t(30.05 * 1e8),  // price
    uint64_t(5000 * 1e8),   // order_quantity
    0,                      // tif
    0,                      // position_effect
    "",                     // order_restrictions
    1,                      // max_price_levels
    0,                      // order_capacity
    {0, ""},                // text
    "0 1",                  // execution_instructions
    0x0001,                 // disclosure_instructions
    0                       // lot_type
};

inline NewOrderRequest tc_om_9 = {
    "18",                  // client_order_id
    "1123",                // submitting_broker_id
    "36",                  // security_id
    8,                     // security_id_source
    "XHKG",                // security_exchange
    "unused",              // broker_location_id
    "unused",              // transaction_time
    1,                     // side
    2,                     // order_type
    uint64_t(30.1 * 1e8),  // price
    uint64_t(5000 * 1e8),  // order_quantity
    3,                     // tif
    0,                     // position_effect
    "",                    // order_restrictions
    0,                     // max_price_levels
    0,                     // order_capacity
    {0, ""},               // text
    "0 1",                 // execution_instructions
    0x0001,                // disclosure_instructions
    0                      // lot_type
};

inline NewOrderRequest tc_om_10 = {
    "19",                   // client_order_id
    "1123",                 // submitting_broker_id
    "36",                   // security_id
    8,                      // security_id_source
    "XHKG",                 // security_exchange
    "unused",               // broker_location_id
    "unused",               // transaction_time
    2,                      // side
    2,                      // order_type
    uint64_t(30.05 * 1e8),  // price
    uint64_t(5000 * 1e8),   // order_quantity
    0,                      // tif
    0,                      // position_effect
    "",                     // order_restrictions
    0,                      // max_price_levels
    0,                      // order_capacity
    {0, ""},                // text
    "0 1",                  // execution_instructions
    0x0001,                 // disclosure_instructions
    0                       // lot_type
};

inline NewOrderRequest tc_om_11 = {
    "20",                  // client_order_id
    "1122",                // submitting_broker_id
    "36",                  // security_id
    8,                     // security_id_source
    "XHKG",                // security_exchange
    "unused",              // broker_location_id
    "unused",              // transaction_time
    2,                     // side
    2,                     // order_type
    uint64_t(30.1 * 1e8),  // price
    uint64_t(4000 * 1e8),  // order_quantity
    0,                     // tif
    0,                     // position_effect
    "",                    // order_restrictions
    0,                     // max_price_levels
    0,                     // order_capacity
    {0, ""},               // text
    "0 1",                 // execution_instructions
    0x0001,                // disclosure_instructions
    0                      // lot_type
};

inline NewOrderRequest tc_om_12 = {
    "21",                  // client_order_id
    "1123",                // submitting_broker_id
    "36",                  // security_id
    8,                     // security_id_source
    "XHKG",                // security_exchange
    "unused",              // broker_location_id
    "unused",              // transaction_time
    1,                     // side
    2,                     // order_type
    uint64_t(30.1 * 1e8),  // price
    uint64_t(1000 * 1e8),  // order_quantity
    0,                     // tif
    0,                     // position_effect
    "",                    // order_restrictions
    0,                     // max_price_levels
    0,                     // order_capacity
    {0, ""},               // text
    "0 1",                 // execution_instructions
    0x0001,                // disclosure_instructions
    0                      // lot_type
};

inline NewOrderRequest tc_om_13 = {
    "22",                  // client_order_id
    "1123",                // submitting_broker_id
    "36",                  // security_id
    8,                     // security_id_source
    "XHKG",                // security_exchange
    "unused",              // broker_location_id
    "unused",              // transaction_time
    1,                     // side
    2,                     // order_type
    uint64_t(30.1 * 1e8),  // price
    uint64_t(1000 * 1e8),  // order_quantity
    3,                     // tif
    0,                     // position_effect
    "",                    // order_restrictions
    0,                     // max_price_levels
    0,                     // order_capacity
    {0, ""},               // text
    "0 1",                 // execution_instructions
    0x0001,                // disclosure_instructions
    0                      // lot_type
};

inline NewOrderRequest tc_om_14 = {
    "23",                  // client_order_id
    "1123",                // submitting_broker_id
    "36",                  // security_id
    8,                     // security_id_source
    "XHKG",                // security_exchange
    "unused",              // broker_location_id
    "unused",              // transaction_time
    2,                     // side
    2,                     // order_type
    uint64_t(30.1 * 1e8),  // price
    uint64_t(1000 * 1e8),  // order_quantity
    0,                     // tif
    0,                     // position_effect
    "",                    // order_restrictions
    1,                     // max_price_levels
    0,                     // order_capacity
    {0, ""},               // text
    "0 1",                 // execution_instructions
    0x0001,                // disclosure_instructions
    0                      // lot_type
};

inline NewOrderRequest tc_om_15 = {
    "24",                   // client_order_id
    "1122",                 // submitting_broker_id
    "36",                   // security_id
    8,                      // security_id_source
    "XHKG",                 // security_exchange
    "unused",               // broker_location_id
    "unused",               // transaction_time
    1,                      // side
    2,                      // order_type
    uint64_t(30.05 * 1e8),  // price
    uint64_t(1000 * 1e8),   // order_quantity
    0,                      // tif
    0,                      // position_effect
    "",                     // order_restrictions
    1,                      // max_price_levels
    0,                      // order_capacity
    {0, ""},                // text
    "0 1",                  // execution_instructions
    0x0001,                 // disclosure_instructions
    0                       // lot_type
};

inline NewOrderRequest tc_om_16 = {
    "25",                  // client_order_id
    "1123",                // submitting_broker_id
    "36",                  // security_id
    8,                     // security_id_source
    "XHKG",                // security_exchange
    "unused",              // broker_location_id
    "unused",              // transaction_time
    1,                     // side
    2,                     // order_type
    uint64_t(30.1 * 1e8),  // price
    uint64_t(3000 * 1e8),  // order_quantity
    0,                     // tif
    0,                     // position_effect
    "",                    // order_restrictions
    0,                     // max_price_levels
    0,                     // order_capacity
    {0, ""},               // text
    "0 1",                 // execution_instructions
    0x0001,                // disclosure_instructions
    0                      // lot_type
};

inline NewOrderRequest tc_om_17 = {
    "26",                   // client_order_id
    "1123",                 // submitting_broker_id
    "36",                   // security_id
    8,                      // security_id_source
    "XHKG",                 // security_exchange
    "unused",               // broker_location_id
    "unused",               // transaction_time
    2,                      // side
    2,                      // order_type
    uint64_t(29.95 * 1e8),  // price
    uint64_t(5000 * 1e8),   // order_quantity
    3,                      // tif
    0,                      // position_effect
    "",                     // order_restrictions
    0,                      // max_price_levels
    0,                      // order_capacity
    {0, ""},                // text
    "",                     // execution_instructions
    0x0001,                 // disclosure_instructions
    0                       // lot_type
};

inline NewOrderRequest tc_om_17_2 = {
    "27",                  // client_order_id
    "1122",                // submitting_broker_id
    "36",                  // security_id
    8,                     // security_id_source
    "XHKG",                // security_exchange
    "unused",              // broker_location_id
    "unused",              // transaction_time
    1,                     // side
    2,                     // order_type
    uint64_t(30 * 1e8),    // price
    uint64_t(3000 * 1e8),  // order_quantity
    0,                     // tif
    0,                     // position_effect
    "",                    // order_restrictions
    0,                     // max_price_levels
    0,                     // order_capacity
    {0, ""},               // text
    "0 1",                 // execution_instructions
    0x0001,                // disclosure_instructions
    0                      // lot_type
};

inline NewOrderRequest tc_om_18 = {
    "30",                  // client_order_id
    "1122",                // submitting_broker_id
    "88",                  // security_id
    8,                     // security_id_source
    "XHKG",                // security_exchange
    "unused",              // broker_location_id
    "unused",              // transaction_time
    1,                     // side
    2,                     // order_type
    uint64_t(15 * 1e8),    // price
    uint64_t(2000 * 1e8),  // order_quantity
    0,                     // tif
    0,                     // position_effect
    "",                    // order_restrictions
    1,                     // max_price_levels
    0,                     // order_capacity
    {0, ""},               // text
    "0 1",                 // execution_instructions
    0x0001,                // disclosure_instructions
    0                      // lot_type
};

inline NewOrderRequest tc_om_19 = {
    "31",                    // client_order_id
    "1122",                  // submitting_broker_id
    "36",                    // security_id
    8,                       // security_id_source
    "XHKG",                  // security_exchange
    "unused",                // broker_location_id
    "unused",                // transaction_time
    1,                       // side
    2,                       // order_type
    uint64_t(29.999 * 1e8),  // price
    uint64_t(2000 * 1e8),    // order_quantity
    0,                       // tif
    0,                       // position_effect
    "",                      // order_restrictions
    0,                       // max_price_levels
    0,                       // order_capacity
    {0, ""},                 // text
    "0 1",                   // execution_instructions
    0x0001,                  // disclosure_instructions
    0                        // lot_type
};

inline NewOrderRequest tc_om_20 = {
    "32",                 // client_order_id
    "1122",               // submitting_broker_id
    "36",                 // security_id
    8,                    // security_id_source
    "XHKG",               // security_exchange
    "unused",             // broker_location_id
    "unused",             // transaction_time
    2,                    // side
    1,                    // order_type
    0,                    // price
    uint64_t(300 * 1e8),  // order_quantity
    9,                    // tif
    0,                    // position_effect
    "",                   // order_restrictions
    0,                    // max_price_levels
    0,                    // order_capacity
    {0, ""},              // text
    "0 1",                // execution_instructions
    0x0001,               // disclosure_instructions
    0                     // lot_type
};

inline NewOrderRequest tc_om_21 = {
    "33",                  // client_order_id
    "1122",                // submitting_broker_id
    "36",                  // security_id
    8,                     // security_id_source
    "XHKG",                // security_exchange
    "unused",              // broker_location_id
    "unused",              // transaction_time
    1,                     // side
    2,                     // order_type
    uint64_t(31.9 * 1e8),  // price
    uint64_t(2000 * 1e8),  // order_quantity
    0,                     // tif
    0,                     // position_effect
    "",                    // order_restrictions
    1,                     // max_price_levels
    0,                     // order_capacity
    {0, ""},               // text
    "0 1",                 // execution_instructions
    0x0001,                // disclosure_instructions
    0                      // lot_type
};

inline NewOrderRequest tc_om_22_s1 = {
    "34",                   // client_order_id
    "1123",                 // submitting_broker_id
    "36",                   // security_id
    8,                      // security_id_source
    "XHKG",                 // security_exchange
    "unused",               // broker_location_id
    "unused",               // transaction_time
    1,                      // side
    2,                      // order_type
    uint64_t(29.95 * 1e8),  // price
    uint64_t(1000 * 1e8),   // order_quantity
    0,                      // tif
    0,                      // position_effect
    "",                     // order_restrictions
    1,                      // max_price_levels
    0,                      // order_capacity
    {0, ""},                // text
    "",                     // execution_instructions
    0x0001,                 // disclosure_instructions
    0                       // lot_type
};

inline CancelRequest tc_om_22_s2 = {
    "35",      // client_order_id
    "1123",    // submitting_broker_id
    "36",      // security_id
    8,         // security_id_source
    "XHKG",    // security_exchange
    "unused",  // broker_location_id
    "unused",  // transaction_time
    1,         // side
    "34",      // original_order_id
    "8888",    // order_id
    {0, ""},   // text
};

inline CancelRequest tc_om_22_s3 = {
    "36",      // client_order_id
    "1123",    // submitting_broker_id
    "36",      // security_id
    8,         // security_id_source
    "XHKG",    // security_exchange
    "unused",  // broker_location_id
    "unused",  // transaction_time
    1,         // side
    "35",      // original_order_id
    "8888",    // order_id
    {0, ""},   // text
};

inline NewOrderRequest tc_om_23_s1 = {
    "37",                  // client_order_id
    "1122",                // submitting_broker_id
    "36",                  // security_id
    8,                     // security_id_source
    "XHKG",                // security_exchange
    "unused",              // broker_location_id
    "unused",              // transaction_time
    1,                     // side
    2,                     // order_type
    uint64_t(30 * 1e8),    // price
    uint64_t(4000 * 1e8),  // order_quantity
    0,                     // tif
    0,                     // position_effect
    "",                    // order_restrictions
    1,                     // max_price_levels
    0,                     // order_capacity
    {0, ""},               // text
    "0 1",                 // execution_instructions
    0x0001,                // disclosure_instructions
    0                      // lot_type
};

inline AmendRequest tc_om_23_s2 = {
    "38",                  // client_order_id
    "1122",                // submitting_broker_id
    "36",                  // security_id
    8,                     // security_id_source
    "XHKG",                // security_exchange
    "unused",              // broker_location_id
    "unused",              // transaction_time
    1,                     // side
    "37",                  // original_client_order_id
    "8888",                // order_id
    2,                     // order_type
    uint64_t(30 * 1e8),    // price
    uint64_t(2000 * 1e8),  // order_quantity
    0,                     // tif
    0,                     // position_effect
    "",                    // order_restrictions
    1,                     // max_price_levels
    0,                     // order_capacity
    {0, ""},               // text
    "0 1",                 // execution_instructions
    0x0001                 // disclosure_instructions
};

inline NewOrderRequest tc_om_24_s1 = {
    "39",                   // client_order_id
    "1122",                 // submitting_broker_id
    "36",                   // security_id
    8,                      // security_id_source
    "XHKG",                 // security_exchange
    "unused",               // broker_location_id
    "unused",               // transaction_time
    1,                      // side
    2,                      // order_type
    uint64_t(29.95 * 1e8),  // price
    uint64_t(3000 * 1e8),   // order_quantity
    0,                      // tif
    0,                      // position_effect
    "",                     // order_restrictions
    1,                      // max_price_levels
    0,                      // order_capacity
    {0, ""},                // text
    "0 1",                  // execution_instructions
    0x0001,                 // disclosure_instructions
    0                       // lot_type
};

inline AmendRequest tc_om_24_s2 = {
    "40",                  // client_order_id
    "1122",                // submitting_broker_id
    "36",                  // security_id
    8,                     // security_id_source
    "XHKG",                // security_exchange
    "unused",              // broker_location_id
    "unused",              // transaction_time
    1,                     // side
    "39",                  // original_client_order_id
    "8888",                // order_id
    2,                     // order_type
    uint64_t(30 * 1e8),    // price
    uint64_t(3000 * 1e8),  // order_quantity
    0,                     // tif
    0,                     // position_effect
    "",                    // order_restrictions
    1,                     // max_price_levels
    0,                     // order_capacity
    {0, ""},               // text
    "0 1",                 // execution_instructions
    0x0001                 // disclosure_instructions
};

inline AmendRequest tc_om_24_s3 = {
    "41",                  // client_order_id
    "1122",                // submitting_broker_id
    "36",                  // security_id
    8,                     // security_id_source
    "XHKG",                // security_exchange
    "unused",              // broker_location_id
    "unused",              // transaction_time
    1,                     // side
    "40",                  // original_client_order_id
    "8888",                // order_id
    2,                     // order_type
    uint64_t(30.2 * 1e8),  // price
    uint64_t(6000 * 1e8),  // order_quantity
    0,                     // tif
    0,                     // position_effect
    "",                    // order_restrictions
    1,                     // max_price_levels
    0,                     // order_capacity
    {0, ""},               // text
    "0 1",                 // execution_instructions
    0x0001                 // disclosure_instructions
};

inline AmendRequest tc_om_25 = {
    "42",                   // client_order_id
    "1122",                 // submitting_broker_id
    "36",                   // security_id
    8,                      // security_id_source
    "XHKG",                 // security_exchange
    "unused",               // broker_location_id
    "unused",               // transaction_time
    1,                      // side
    "80",                   // original_client_order_id
    "8888",                 // order_id
    2,                      // order_type
    uint64_t(30.25 * 1e8),  // price
    uint64_t(5000 * 1e8),   // order_quantity
    0,                      // tif
    0,                      // position_effect
    "",                     // order_restrictions
    1,                      // max_price_levels
    0,                      // order_capacity
    {0, ""},                // text
    "0 1",                  // execution_instructions
    0x0001                  // disclosure_instructions
};

inline NewOrderRequest tc_om_26_s1 = {
    "43",                  // client_order_id
    "1122",                // submitting_broker_id
    "36",                  // security_id
    8,                     // security_id_source
    "XHKG",                // security_exchange
    "unused",              // broker_location_id
    "unused",              // transaction_time
    1,                     // side
    2,                     // order_type
    uint64_t(29.9 * 1e8),  // price
    uint64_t(1000 * 1e8),  // order_quantity
    0,                     // tif
    0,                     // position_effect
    "",                    // order_restrictions
    1,                     // max_price_levels
    0,                     // order_capacity
    {0, ""},               // text
    "0 1",                 // execution_instructions
    0x0001,                // disclosure_instructions
    0                      // lot_type
};

inline NewOrderRequest tc_om_26_s2 = {
    "44",                  // client_order_id
    "1122",                // submitting_broker_id
    "37",                  // security_id
    8,                     // security_id_source
    "XHKG",                // security_exchange
    "unused",              // broker_location_id
    "unused",              // transaction_time
    2,                     // side
    2,                     // order_type
    uint64_t(50.1 * 1e8),  // price
    uint64_t(1000 * 1e8),  // order_quantity
    0,                     // tif
    0,                     // position_effect
    "",                    // order_restrictions
    1,                     // max_price_levels
    0,                     // order_capacity
    {0, ""},               // text
    "0 1",                 // execution_instructions
    0x0001,                // disclosure_instructions
    0                      // lot_type
};

inline MassCancelRequest tc_om_26_s3 = {
    "45",      // client_order_id
    "1122",    // submitting_broker_id
    "",        // security_id
    0,         // security_id_source
    "",        // security_exchange
    "unused",  // broker_location_id
    "unused",  // transaction_time
    0,         // side
    7,         // mass_cancel_request_type
    "",        // market segment id
};

inline NewOrderRequest tc_om_27_s1 = {
    "46",                  // client_order_id
    "1122",                // submitting_broker_id
    "36",                  // security_id
    8,                     // security_id_source
    "XHKG",                // security_exchange
    "unused",              // broker_location_id
    "unused",              // transaction_time
    1,                     // side
    2,                     // order_type
    uint64_t(29.9 * 1e8),  // price
    uint64_t(1000 * 1e8),  // order_quantity
    0,                     // tif
    0,                     // position_effect
    "",                    // order_restrictions
    1,                     // max_price_levels
    0,                     // order_capacity
    {0, ""},               // text
    "0 1",                 // execution_instructions
    0x0001,                // disclosure_instructions
    0                      // lot_type
};

inline NewOrderRequest tc_om_27_s2 = {
    "47",                  // client_order_id
    "1122",                // submitting_broker_id
    "38",                  // security_id
    8,                     // security_id_source
    "XHKG",                // security_exchange
    "unused",              // broker_location_id
    "unused",              // transaction_time
    2,                     // side
    2,                     // order_type
    uint64_t(50.1 * 1e8),  // price
    uint64_t(1000 * 1e8),  // order_quantity
    0,                     // tif
    0,                     // position_effect
    "",                    // order_restrictions
    1,                     // max_price_levels
    0,                     // order_capacity
    {0, ""},               // text
    "0 1",                 // execution_instructions
    0x0001,                // disclosure_instructions
    0                      // lot_type
};

inline MassCancelRequest tc_om_27_s3 = {
    "48",      // client_order_id
    "1122",    // submitting_broker_id
    "",        // security_id
    0,         // security_id_source
    "",        // security_exchange
    "unused",  // broker_location_id
    "unused",  // transaction_time
    0,         // side
    9,         // mass_cancel_request_type
    "MAIN",    // market segment id
};

inline NewOrderRequest tc_om_28_s1 = {
    "49",                  // client_order_id
    "1122",                // submitting_broker_id
    "38",                  // security_id
    8,                     // security_id_source
    "XHKG",                // security_exchange
    "unused",              // broker_location_id
    "unused",              // transaction_time
    1,                     // side
    2,                     // order_type
    uint64_t(29.9 * 1e8),  // price
    uint64_t(1000 * 1e8),  // order_quantity
    0,                     // tif
    0,                     // position_effect
    "",                    // order_restrictions
    1,                     // max_price_levels
    0,                     // order_capacity
    {0, ""},               // text
    "0 1",                 // execution_instructions
    0x0001,                // disclosure_instructions
    0                      // lot_type
};

inline NewOrderRequest tc_om_28_s2 = {
    "50",                  // client_order_id
    "1122",                // submitting_broker_id
    "38",                  // security_id
    8,                     // security_id_source
    "XHKG",                // security_exchange
    "unused",              // broker_location_id
    "unused",              // transaction_time
    2,                     // side
    2,                     // order_type
    uint64_t(30 * 1e8),    // price
    uint64_t(1000 * 1e8),  // order_quantity
    0,                     // tif
    0,                     // position_effect
    "",                    // order_restrictions
    1,                     // max_price_levels
    0,                     // order_capacity
    {0, ""},               // text
    "0 1",                 // execution_instructions
    0x0001,                // disclosure_instructions
    0                      // lot_type
};

inline MassCancelRequest tc_om_28_s3 = {
    "51",      // client_order_id
    "1122",    // submitting_broker_id
    "38",      // security_id
    8,         // security_id_source
    "XHKG",    // security_exchange
    "unused",  // broker_location_id
    "unused",  // transaction_time
    0,         // side
    1,         // mass_cancel_request_type
    "",        // market segment id
};

inline MassCancelRequest tc_om_29 = {
    "52",      // client_order_id
    "1122",    // submitting_broker_id
    "",        // security_id
    0,         // security_id_source
    "",        // security_exchange
    "unused",  // broker_location_id
    "unused",  // transaction_time
    0,         // side
    9,         // mass_cancel_request_type
    "ABCD",    // market segment id
};

inline NewOrderRequest tc_om_30_s1 = {
    "53",                  // client_order_id
    "1122",                // submitting_broker_id
    "36",                  // security_id
    8,                     // security_id_source
    "XHKG",                // security_exchange
    "unused",              // broker_location_id
    "unused",              // transaction_time
    1,                     // side
    2,                     // order_type
    uint64_t(29.9 * 1e8),  // price
    uint64_t(1000 * 1e8),  // order_quantity
    0,                     // tif
    0,                     // position_effect
    "",                    // order_restrictions
    1,                     // max_price_levels
    0,                     // order_capacity
    {0, ""},               // text
    "0 1",                 // execution_instructions
    0x0001,                // disclosure_instructions
    0                      // lot_type
};

inline OboCancelRequest tc_om_30_s2 = {
    "54",        // client_order_id
    "1200",      // submitting_broker_id
    "36",        // security_id
    8,           // security_id_source
    "XHKG",      // security_exchange
    "unused",    // broker_location_id
    "unused",    // transaction_time
    1,           // side
    "",          // original_order_id
    "35508220",  // order_id
    "1122",      // owning_broker_id
    {0, ""},     // text
};

inline NewOrderRequest tc_om_31_s1 = {
    "55",                  // client_order_id
    "1122",                // submitting_broker_id
    "36",                  // security_id
    8,                     // security_id_source
    "XHKG",                // security_exchange
    "unused",              // broker_location_id
    "unused",              // transaction_time
    1,                     // side
    2,                     // order_type
    uint64_t(29.9 * 1e8),  // price
    uint64_t(1000 * 1e8),  // order_quantity
    0,                     // tif
    0,                     // position_effect
    "",                    // order_restrictions
    1,                     // max_price_levels
    0,                     // order_capacity
    {0, ""},               // text
    "0 1",                 // execution_instructions
    0x0001,                // disclosure_instructions
    0                      // lot_type
};

inline NewOrderRequest tc_om_31_s2 = {
    "56",                  // client_order_id
    "1122",                // submitting_broker_id
    "38",                  // security_id
    8,                     // security_id_source
    "XHKG",                // security_exchange
    "unused",              // broker_location_id
    "unused",              // transaction_time
    2,                     // side
    2,                     // order_type
    uint64_t(50.1 * 1e8),  // price
    uint64_t(1000 * 1e8),  // order_quantity
    0,                     // tif
    0,                     // position_effect
    "",                    // order_restrictions
    1,                     // max_price_levels
    0,                     // order_capacity
    {0, ""},               // text
    "0 1",                 // execution_instructions
    0x0001,                // disclosure_instructions
    0                      // lot_type
};

inline OboMassCancelRequest tc_om_31_s3 = {
    "57",      // client_order_id
    "1200",    // submitting_broker_id
    "",        // security_id
    0,         // security_id_source
    "",        // security_exchange
    "unused",  // broker_location_id
    "unused",  // transaction_time
    0,         // side
    7,         // mass_cancel_request_type
    "",        // market segment id
    "1122",    // owning_broker_id
};

inline NewOrderRequest tc_om_32_s1 = {
    "58",                  // client_order_id
    "1122",                // submitting_broker_id
    "36",                  // security_id
    8,                     // security_id_source
    "XHKG",                // security_exchange
    "unused",              // broker_location_id
    "unused",              // transaction_time
    1,                     // side
    2,                     // order_type
    uint64_t(29.9 * 1e8),  // price
    uint64_t(1000 * 1e8),  // order_quantity
    0,                     // tif
    0,                     // position_effect
    "",                    // order_restrictions
    1,                     // max_price_levels
    0,                     // order_capacity
    {0, ""},               // text
    "0 1",                 // execution_instructions
    0x0001,                // disclosure_instructions
    0                      // lot_type
};

inline OboCancelRequest tc_om_32_s2 = {
    "59",        // client_order_id
    "1300",      // submitting_broker_id
    "36",        // security_id
    8,           // security_id_source
    "XHKG",      // security_exchange
    "unused",    // broker_location_id
    "unused",    // transaction_time
    1,           // side
    "",          // original_order_id
    "35513730",  // order_id
    "1122",      // owning_broker_id
    {0, ""},     // text
};

inline OboMassCancelRequest tc_om_33 = {
    "60",      // client_order_id
    "1400",    // submitting_broker_id
    "",        // security_id
    0,         // security_id_source
    "",        // security_exchange
    "unused",  // broker_location_id
    "unused",  // transaction_time
    0,         // side
    7,         // mass_cancel_request_type
    "",        // market segment id
    "1122",    // owning_broker_id
};

inline QuoteRequest tc_qm_01_s1 = {
    "1200",                // submitting_broker_id
    "unused",              // broker_location_id
    "40",                  // security_id
    8,                     // security_id_source
    "XHKG",                // security_exchange
    "86",                  // quote_bid_id
    "87",                  // quote_offer_id
    1,                     // quote_type
    0,                     // side
    uint64_t(5000 * 1e8),  // bid_size
    uint64_t(4000 * 1e8),  // offer_size
    uint64_t(15 * 1e8),    // bid_price
    uint64_t(15.1 * 1e8),  // offer_price
    "unused",              // transaction_time
    0,                     // position_effect
    "",                    // order_restrictions
    {0, ""},               // text
    "0 1"                  // execution instructions
};

inline QuoteRequest tc_qm_01_s2 = {
    "1200",                // submitting_broker_id
    "unused",              // broker_location_id
    "40",                  // security_id
    8,                     // security_id_source
    "XHKG",                // security_exchange
    "88",                  // quote_bid_id
    "89",                  // quote_offer_id
    1,                     // quote_type
    0,                     // side
    uint64_t(2000 * 1e8),  // bid_size
    uint64_t(4000 * 1e8),  // offer_size
    uint64_t(15 * 1e8),    // bid_price
    uint64_t(15.1 * 1e8),  // offer_price
    "unused",              // transaction_time
    0,                     // position_effect
    "",                    // order_restrictions
    {0, ""},               // text
    "0 1"                  // execution instructions
};

inline QuoteRequest tc_qm_01_s3 = {
    "1200",                // submitting_broker_id
    "unused",              // broker_location_id
    "40",                  // security_id
    8,                     // security_id_source
    "XHKG",                // security_exchange
    "90",                  // quote_bid_id
    "91",                  // quote_offer_id
    1,                     // quote_type
    0,                     // side
    uint64_t(2000 * 1e8),  // bid_size
    uint64_t(6000 * 1e8),  // offer_size
    uint64_t(15 * 1e8),    // bid_price
    uint64_t(15.1 * 1e8),  // offer_price
    "unused",              // transaction_time
    0,                     // position_effect
    "",                    // order_restrictions
    {0, ""},               // text
    "0 1"                  // execution instructions
};

inline QuoteRequest tc_qm_02 = {
    "1200",                // submitting_broker_id
    "unused",              // broker_location_id
    "88",                  // security_id
    8,                     // security_id_source
    "XHKG",                // security_exchange
    "92",                  // quote_bid_id
    "93",                  // quote_offer_id
    1,                     // quote_type
    0,                     // side
    uint64_t(3000 * 1e8),  // bid_size
    uint64_t(5000 * 1e8),  // offer_size
    uint64_t(10 * 1e8),    // bid_price
    uint64_t(10.1 * 1e8),  // offer_price
    "unused",              // transaction_time
    0,                     // position_effect
    "",                    // order_restrictions
    {0, ""},               // text
    "0 1"                  // execution instructions
};

inline QuoteRequest tc_qm_03_s1 = {
    "1200",                // submitting_broker_id
    "unused",              // broker_location_id
    "40",                  // security_id
    8,                     // security_id_source
    "XHKG",                // security_exchange
    "94",                  // quote_bid_id
    "95",                  // quote_offer_id
    1,                     // quote_type
    0,                     // side
    uint64_t(5000 * 1e8),  // bid_size
    uint64_t(6000 * 1e8),  // offer_size
    uint64_t(15 * 1e8),    // bid_price
    uint64_t(15.1 * 1e8),  // offer_price
    "unused",              // transaction_time
    0,                     // position_effect
    "",                    // order_restrictions
    {0, ""},               // text
    "0 1"                  // execution instructions
};

inline QuoteCancelRequest tc_qm_03_s2 = {
    "1200",    // submitting_broker_id
    "unused",  // broker_location_id
    "40",      // security_id
    8,         // security_id_source
    "XHKG",    // security_exchange
    "96",      // quote_message_id
    1          // quote_cancel_type
};

inline QuoteCancelRequest tc_qm_04 = {
    "1200",    // submitting_broker_id
    "unused",  // broker_location_id
    "88",      // security_id
    8,         // security_id_source
    "XHKG",    // security_exchange
    "97",      // quote_message_id
    1          // quote_cancel_type
};

inline TradeCaptureReport tc_tm_01 = {
    "200",                 // trade_report_id
    0,                     // trade_report_trans_type
    0,                     // trade_report_type
    1,                     // trade_handling_instructions
    "1200",                // submitting_broker_id
    "1300",                // counterparty_broker_id
    "unused",              // broker_location_id
    "41",                  // security_id
    8,                     // security_id_source
    "XHKG",                // security_exchange
    2,                     // side
    "unused",              // transaction_time
    "",                    // trade_id
    4,                     // trade_type,
    uint64_t(5000 * 1e8),  // execution_quantity
    uint64_t(30 * 1e8),    // execution_price
    0,                     // clearing_instruction
    0,                     // position_effect
    0,                     // order_capacity
    0,                     // order_category
    {0, ""},               // text
    "0 1",                 // execution_instructions
    0,                     // exec_type
    0,
    0,
    ""  // order_id
};

inline TradeCaptureReport tc_tm_02 = {
    "201",                 // trade_report_id
    0,                     // trade_report_trans_type
    0,                     // trade_report_type
    1,                     // trade_handling_instructions
    "1200",                // submitting_broker_id
    "1300",                // counterparty_broker_id
    "unused",              // broker_location_id
    "88",                  // security_id
    8,                     // security_id_source
    "XHKG",                // security_exchange
    2,                     // side
    "unused",              // transaction_time
    "",                    // trade_id
    4,                     // trade_type,
    uint64_t(5000 * 1e8),  // execution_quantity
    uint64_t(30 * 1e8),    // execution_price
    0,                     // clearing_instruction
    0,                     // position_effect
    0,                     // order_capacity
    0,                     // order_category
    {0, ""},               // text
    "0 1",                 // execution_instructions
    0,                     // exec_type
    0,
    0,
    ""  // order_id
};

inline NewOrderRequest tc_in_01_s1 = {
    "205",                 // client_order_id
    "1122",                // submitting_broker_id
    "42",                  // security_id
    8,                     // security_id_source
    "XHKG",                // security_exchange
    "unused",              // broker_location_id
    "unused",              // transaction_time
    1,                     // side
    2,                     // order_type
    uint64_t(30 * 1e8),    // price
    uint64_t(4000 * 1e8),  // order_quantity
    0,                     // tif
    0,                     // position_effect
    "",                    // order_restrictions
    1,                     // max_price_levels
    0,                     // order_capacity
    {0, ""},               // text
    "0 1",                 // execution_instructions
    0x0001,                // disclosure_instructions
    0                      // lot_type
};

inline NewOrderRequest tc_in_01_s2 = {
    "206",                  // client_order_id
    "1122",                 // submitting_broker_id
    "42",                   // security_id
    8,                      // security_id_source
    "XHKG",                 // security_exchange
    "unused",               // broker_location_id
    "unused",               // transaction_time
    1,                      // side
    2,                      // order_type
    uint64_t(29.95 * 1e8),  // price
    uint64_t(4000 * 1e8),   // order_quantity
    0,                      // tif
    0,                      // position_effect
    "",                     // order_restrictions
    0,                      // max_price_levels
    0,                      // order_capacity
    {0, ""},                // text
    "0 1",                  // execution_instructions
    0x0001,                 // disclosure_instructions
    0                       // lot_type
};

inline NewOrderRequest tc_in_01_s3 = {
    "207",                 // client_order_id
    "1122",                // submitting_broker_id
    "42",                  // security_id
    8,                     // security_id_source
    "XHKG",                // security_exchange
    "unused",              // broker_location_id
    "unused",              // transaction_time
    1,                     // side
    2,                     // order_type
    uint64_t(29.9 * 1e8),  // price
    uint64_t(4000 * 1e8),  // order_quantity
    0,                     // tif
    0,                     // position_effect
    "",                    // order_restrictions
    0,                     // max_price_levels
    0,                     // order_capacity
    {0, ""},               // text
    "0 1",                 // execution_instructions
    0x0001,                // disclosure_instructions
    0                      // lot_type
};

inline NewOrderRequest tc_ol_01_s1 = {
    "1001",              // client_order_id
    "1122",              // submitting_broker_id
    "41",                // security_id
    8,                   // security_id_source
    "XHKG",              // security_exchange
    "unused",            // broker_location_id
    "unused",            // transaction_time
    1,                   // side
    2,                   // order_type
    uint64_t(30 * 1e8),  // price
    uint64_t(30 * 1e8),  // order_quantity
    0,                   // tif
    0,                   // position_effect
    "",                  // order_restrictions
    0,                   // max_price_levels
    0,                   // order_capacity
    {0, ""},             // text
    "1",                 // execution_instructions
    0x0001,              // disclosure_instructions
    1                    // lot_type
};

inline NewOrderRequest tc_ol_01_s2 = {
    "1002",                 // client_order_id
    "1122",                 // submitting_broker_id
    "41",                   // security_id
    8,                      // security_id_source
    "XHKG",                 // security_exchange
    "unused",               // broker_location_id
    "unused",               // transaction_time
    2,                      // side
    2,                      // order_type
    uint64_t(30.32 * 1e8),  // price
    uint64_t(40 * 1e8),     // order_quantity
    0,                      // tif
    0,                      // position_effect
    "",                     // order_restrictions
    0,                      // max_price_levels
    0,                      // order_capacity
    {0, ""},                // text
    "1",                    // execution_instructions
    0x0001,                 // disclosure_instructions
    1                       // lot_type
};

inline CancelRequest tc_ol_01_s3 = {
    "1003",    // client_order_id
    "1122",    // submitting_broker_id
    "41",      // security_id
    8,         // security_id_source
    "XHKG",    // security_exchange
    "unused",  // broker_location_id
    "unused",  // transaction_time
    1,         // side
    "1001",    // original_order_id
    "12345",   // order_id
    {0, ""},   // text
};

inline NewOrderRequest tc_ol_01_s4 = {
    "1004",                 // client_order_id
    "1122",                 // submitting_broker_id
    "42",                   // security_id
    8,                      // security_id_source
    "XHKG",                 // security_exchange
    "unused",               // broker_location_id
    "unused",               // transaction_time
    1,                      // side
    2,                      // order_type
    uint64_t(0.001 * 1e8),  // price
    uint64_t(1003 * 1e8),   // order_quantity
    0,                      // tif
    0,                      // position_effect
    "",                     // order_restrictions
    0,                      // max_price_levels
    0,                      // order_capacity
    {0, ""},                // text
    "1",                    // execution_instructions
    0x0001,                 // disclosure_instructions
    1                       // lot_type
};

inline NewOrderRequest tc_ol_01_s5 = {
    "1005",                 // client_order_id
    "1122",                 // submitting_broker_id
    "42",                   // security_id
    8,                      // security_id_source
    "XHKG",                 // security_exchange
    "unused",               // broker_location_id
    "unused",               // transaction_time
    2,                      // side
    2,                      // order_type
    uint64_t(0.009 * 1e8),  // price
    uint64_t(1503 * 1e8),   // order_quantity
    0,                      // tif
    0,                      // position_effect
    "",                     // order_restrictions
    0,                      // max_price_levels
    0,                      // order_capacity
    {0, ""},                // text
    "1",                    // execution_instructions
    0x0001,                 // disclosure_instructions
    1                       // lot_type
};

inline CancelRequest tc_ol_01_s6 = {
    "1006",    // client_order_id
    "1122",    // submitting_broker_id
    "42",      // security_id
    8,         // security_id_source
    "XHKG",    // security_exchange
    "unused",  // broker_location_id
    "unused",  // transaction_time
    1,         // side
    "1004",    // original_order_id
    "12345",   // order_id
    {0, ""},   // text
};

inline NewOrderRequest tc_ol_02 = {
    "1007",                 // client_order_id
    "1122",                 // submitting_broker_id
    "37",                   // security_id
    8,                      // security_id_source
    "XHKG",                 // security_exchange
    "unused",               // broker_location_id
    "unused",               // transaction_time
    1,                      // side
    2,                      // order_type
    uint64_t(29.95 * 1e8),  // price
    uint64_t(50 * 1e8),     // order_quantity
    0,                      // tif
    0,                      // position_effect
    "",                     // order_restrictions
    0,                      // max_price_levels
    0,                      // order_capacity
    {0, ""},                // text
    "1",                    // execution_instructions
    0x0001,                 // disclosure_instructions
    1                       // lot_type
};

inline NewOrderRequest tc_ol_03_s1 = {
    "1008",              // client_order_id
    "1122",              // submitting_broker_id
    "37",                // security_id
    8,                   // security_id_source
    "XHKG",              // security_exchange
    "unused",            // broker_location_id
    "unused",            // transaction_time
    1,                   // side
    2,                   // order_type
    uint64_t(30 * 1e8),  // price
    uint64_t(30 * 1e8),  // order_quantity
    0,                   // tif
    0,                   // position_effect
    "",                  // order_restrictions
    0,                   // max_price_levels
    0,                   // order_capacity
    {0, ""},             // text
    "1",                 // execution_instructions
    0x0001,              // disclosure_instructions
    1                    // lot_type
};

inline TradeCaptureReport tc_ol_03_s2 = {
    "1009",              // trade_report_id
    0,                   // trade_report_trans_type
    0,                   // trade_report_type
    1,                   // trade_handling_instructions
    "1123",              // submitting_broker_id
    "1122",              // counterparty_broker_id
    "unused",            // broker_location_id
    "37",                // security_id
    8,                   // security_id_source
    "XHKG",              // security_exchange
    2,                   // side
    "unused",            // transaction_time
    "",                  // trade_id
    102,                 // trade_type,
    uint64_t(30 * 1e8),  // execution_quantity
    uint64_t(30 * 1e8),  // execution_price
    0,                   // clearing_instruction
    0,                   // position_effect
    0,                   // order_capacity
    0,                   // order_category
    {0, ""},             // text
    "1",                 // execution_instructions
    0,                   // exec_type
    0,
    0,
    "800301"  // order_id
};

inline NewOrderRequest tc_ol_04_s1 = {
    "1010",                 // client_order_id
    "1122",                 // submitting_broker_id
    "37",                   // security_id
    8,                      // security_id_source
    "XHKG",                 // security_exchange
    "unused",               // broker_location_id
    "unused",               // transaction_time
    1,                      // side
    2,                      // order_type
    uint64_t(0.009 * 1e8),  // price
    uint64_t(1002 * 1e8),   // order_quantity
    0,                      // tif
    0,                      // position_effect
    "",                     // order_restrictions
    0,                      // max_price_levels
    0,                      // order_capacity
    {0, ""},                // text
    "1",                    // execution_instructions
    0x0001,                 // disclosure_instructions
    1                       // lot_type
};

inline TradeCaptureReport tc_ol_04_s2 = {
    "1011",                 // trade_report_id
    0,                      // trade_report_trans_type
    0,                      // trade_report_type
    1,                      // trade_handling_instructions
    "1123",                 // submitting_broker_id
    "1122",                 // counterparty_broker_id
    "unused",               // broker_location_id
    "37",                   // security_id
    8,                      // security_id_source
    "XHKG",                 // security_exchange
    2,                      // side
    "unused",               // transaction_time
    "",                     // trade_id
    102,                    // trade_type,
    uint64_t(1002 * 1e8),   // execution_quantity
    uint64_t(0.009 * 1e8),  // execution_price
    0,                      // clearing_instruction
    0,                      // position_effect
    0,                      // order_capacity
    0,                      // order_category
    {0, ""},                // text
    "1",                    // execution_instructions
    0,                      // exec_type
    0,
    0,
    "800301"  // order_id
};

inline NewOrderRequest tc_ol_05_s1 = {
    "1012",                // client_order_id
    "1122",                // submitting_broker_id
    "37",                  // security_id
    8,                     // security_id_source
    "XHKG",                // security_exchange
    "unused",              // broker_location_id
    "unused",              // transaction_time
    2,                     // side
    2,                     // order_type
    uint64_t(30 * 1e8),    // price
    uint64_t(1000 * 1e8),  // order_quantity
    0,                     // tif
    0,                     // position_effect
    "",                    // order_restrictions
    0,                     // max_price_levels
    0,                     // order_capacity
    {0, ""},               // text
    "1",                   // execution_instructions
    0x0001,                // disclosure_instructions
    1                      // lot_type
};

inline NewOrderRequest tc_ol_05_s2 = {
    "1013",                // client_order_id
    "1122",                // submitting_broker_id
    "42",                  // security_id
    8,                     // security_id_source
    "XHKG",                // security_exchange
    "unused",              // broker_location_id
    "unused",              // transaction_time
    2,                     // side
    2,                     // order_type
    uint64_t(0.01 * 1e8),  // price
    uint64_t(1000 * 1e8),  // order_quantity
    0,                     // tif
    0,                     // position_effect
    "",                    // order_restrictions
    0,                     // max_price_levels
    0,                     // order_capacity
    {0, ""},               // text
    "1",                   // execution_instructions
    0x0001,                // disclosure_instructions
    1                      // lot_type
};

inline TradeCaptureReport tc_ol_06_s1 = {
    "1014",              // trade_report_id
    0,                   // trade_report_trans_type
    0,                   // trade_report_type
    1,                   // trade_handling_instructions
    "1123",              // submitting_broker_id
    "1122",              // counterparty_broker_id
    "unused",            // broker_location_id
    "37",                // security_id
    8,                   // security_id_source
    "XHKG",              // security_exchange
    1,                   // side
    "unused",            // transaction_time
    "",                  // trade_id
    102,                 // trade_type,
    uint64_t(30 * 1e8),  // execution_quantity
    uint64_t(29 * 1e8),  // execution_price
    0,                   // clearing_instruction
    0,                   // position_effect
    0,                   // order_capacity
    0,                   // order_category
    {0, ""},             // text
    "1",                 // execution_instructions
    0,                   // exec_type
    0,
    0,
    "333"  // order_id
};

inline TradeCaptureReport tc_ol_06_s2 = {
    "1015",                 // trade_report_id
    0,                      // trade_report_trans_type
    0,                      // trade_report_type
    1,                      // trade_handling_instructions
    "1123",                 // submitting_broker_id
    "1122",                 // counterparty_broker_id
    "unused",               // broker_location_id
    "42",                   // security_id
    8,                      // security_id_source
    "XHKG",                 // security_exchange
    1,                      // side
    "unused",               // transaction_time
    "",                     // trade_id
    102,                    // trade_type,
    uint64_t(1002 * 1e8),   // execution_quantity
    uint64_t(0.001 * 1e8),  // execution_price
    0,                      // clearing_instruction
    0,                      // position_effect
    0,                      // order_capacity
    0,                      // order_category
    {0, ""},                // text
    "1",                    // execution_instructions
    0,                      // exec_type
    0,
    0,
    "444"  // order_id
};

#endif  // OCG_BSS_TESTCASE_H_
