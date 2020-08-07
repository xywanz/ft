// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef BSS_TEST_TESTCASE_H_
#define BSS_TEST_TESTCASE_H_

#include <vector>

#include "protocol/protocol.h"

using namespace ft::bss;

std::vector<ExecutionReport> tc_accepted_and_filled(const NewOrderRequest& req);

std::vector<ExecutionReport> tc_accepted_and_several_traded(
    const NewOrderRequest& req);

#endif  // BSS_TEST_TESTCASE_H_
