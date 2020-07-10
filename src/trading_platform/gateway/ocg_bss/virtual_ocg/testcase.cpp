// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "virtual_ocg/testcase.h"

#include <algorithm>
#include <cstdio>
#include <cstring>

using namespace ft::bss;

static uint32_t next_order_id() {
  static uint32_t __next_order_id = 1;
  return __next_order_id++;
}

static uint32_t next_execution_id() {
  static uint32_t __next_execution_id = 1;
  return __next_execution_id++;
}

std::vector<ExecutionReport> tc_accepted_and_filled(
    const NewOrderRequest& req) {
  std::vector<ExecutionReport> reports;
  // 先回一个Accepted
  uint32_t order_id = next_order_id();
  {
    ExecutionReport report{};
    strncpy(report.client_order_id, req.client_order_id,
            sizeof(report.client_order_id));
    strncpy(report.submitting_broker_id, req.submitting_broker_id,
            sizeof(report.submitting_broker_id));
    strncpy(report.security_id, req.security_id, sizeof(report.security_id));
    report.security_id_source = req.security_id_source;
    if (req.security_exchange[0] != 0)
      strncpy(report.security_exchange, req.security_exchange,
              sizeof(report.security_exchange));
    if (req.broker_location_id[0] != 0)
      strncpy(report.broker_location_id, req.broker_location_id,
              sizeof(report.broker_location_id));
    strncpy(report.transaction_time, req.transaction_time,
            sizeof(report.transaction_time));
    report.side = req.side;
    snprintf(report.order_id, sizeof(report.order_id), "%u", order_id);
    report.order_type = req.order_type;
    report.price = req.price;
    report.order_quantity = req.order_quantity;
    report.tif = req.tif;
    report.position_effect = req.position_effect;
    strncpy(report.order_restrictions, req.order_restrictions,
            sizeof(report.order_restrictions));
    report.max_price_levels = req.max_price_levels;
    report.order_capacity = req.order_capacity;
    snprintf(report.execution_id, sizeof(report.execution_id), "%u",
             next_execution_id());
    report.order_status = 0;
    report.exec_type = EXEC_TYPE_NEW;
    report.cumulative_quantity = 0;
    report.leaves_quantity = req.order_quantity;
    report.lot_type = req.lot_type;

    reports.emplace_back(report);
  }

  // 再回一个全部成交
  {
    ExecutionReport report{};
    strncpy(report.client_order_id, req.client_order_id,
            sizeof(report.client_order_id));
    strncpy(report.submitting_broker_id, req.submitting_broker_id,
            sizeof(report.submitting_broker_id));
    strncpy(report.security_id, req.security_id, sizeof(report.security_id));
    report.security_id_source = req.security_id_source;
    if (req.security_exchange[0] != 0)
      strncpy(report.security_exchange, req.security_exchange,
              sizeof(report.security_exchange));
    if (req.broker_location_id[0] != 0)
      strncpy(report.broker_location_id, req.broker_location_id,
              sizeof(report.broker_location_id));
    strncpy(report.transaction_time, req.transaction_time,
            sizeof(report.transaction_time));
    report.side = req.side;
    snprintf(report.order_id, sizeof(report.order_id), "%u", order_id);
    report.order_type = req.order_type;
    report.price = req.price;
    report.order_quantity = req.order_quantity;
    report.tif = req.tif;
    report.position_effect = req.position_effect;
    strncpy(report.order_restrictions, req.order_restrictions,
            sizeof(report.order_restrictions));
    report.max_price_levels = req.max_price_levels;
    report.order_capacity = req.order_capacity;
    snprintf(report.execution_id, sizeof(report.execution_id), "%u",
             next_execution_id());
    report.order_status = 2;
    report.exec_type = EXEC_TYPE_TRADE;
    report.cumulative_quantity = req.order_quantity;
    report.leaves_quantity = 0;
    report.lot_type = req.lot_type;
    report.execution_quantity = req.order_quantity;
    report.execution_price = req.price;

    reports.emplace_back(report);
  }

  return reports;
}

std::vector<ExecutionReport> tc_accepted_and_several_traded(
    const NewOrderRequest& req) {
  std::vector<ExecutionReport> reports;
  // 先回一个Accepted
  uint32_t order_id = next_order_id();
  {
    ExecutionReport report{};
    strncpy(report.client_order_id, req.client_order_id,
            sizeof(report.client_order_id));
    strncpy(report.submitting_broker_id, req.submitting_broker_id,
            sizeof(report.submitting_broker_id));
    strncpy(report.security_id, req.security_id, sizeof(report.security_id));
    report.security_id_source = req.security_id_source;
    if (req.security_exchange[0] != 0)
      strncpy(report.security_exchange, req.security_exchange,
              sizeof(report.security_exchange));
    if (req.broker_location_id[0] != 0)
      strncpy(report.broker_location_id, req.broker_location_id,
              sizeof(report.broker_location_id));
    strncpy(report.transaction_time, req.transaction_time,
            sizeof(report.transaction_time));
    report.side = req.side;
    snprintf(report.order_id, sizeof(report.order_id), "%u", order_id);
    report.order_type = req.order_type;
    report.price = req.price;
    report.order_quantity = req.order_quantity;
    report.tif = req.tif;
    report.position_effect = req.position_effect;
    strncpy(report.order_restrictions, req.order_restrictions,
            sizeof(report.order_restrictions));
    report.max_price_levels = req.max_price_levels;
    report.order_capacity = req.order_capacity;
    snprintf(report.execution_id, sizeof(report.execution_id), "%u",
             next_execution_id());
    report.order_status = 0;
    report.exec_type = EXEC_TYPE_NEW;
    report.cumulative_quantity = 0;
    report.leaves_quantity = req.order_quantity;
    report.lot_type = req.lot_type;

    reports.emplace_back(report);
  }

  // 再回多个成交
  uint64_t quantity = req.order_quantity / 100000000UL;
  for (uint64_t traded = 0; traded < quantity;) {
    uint32_t this_traded = std::min(100UL, quantity - traded);
    printf("this_traded: %u\n", this_traded);
    traded += this_traded;
    ExecutionReport report{};
    strncpy(report.client_order_id, req.client_order_id,
            sizeof(report.client_order_id));
    strncpy(report.submitting_broker_id, req.submitting_broker_id,
            sizeof(report.submitting_broker_id));
    strncpy(report.security_id, req.security_id, sizeof(report.security_id));
    report.security_id_source = req.security_id_source;
    if (req.security_exchange[0] != 0)
      strncpy(report.security_exchange, req.security_exchange,
              sizeof(report.security_exchange));
    if (req.broker_location_id[0] != 0)
      strncpy(report.broker_location_id, req.broker_location_id,
              sizeof(report.broker_location_id));
    strncpy(report.transaction_time, req.transaction_time,
            sizeof(report.transaction_time));
    report.side = req.side;
    snprintf(report.order_id, sizeof(report.order_id), "%u", order_id);
    report.order_type = req.order_type;
    report.price = req.price;
    report.order_quantity = req.order_quantity;
    report.tif = req.tif;
    report.position_effect = req.position_effect;
    strncpy(report.order_restrictions, req.order_restrictions,
            sizeof(report.order_restrictions));
    report.max_price_levels = req.max_price_levels;
    report.order_capacity = req.order_capacity;
    snprintf(report.execution_id, sizeof(report.execution_id), "%u",
             next_execution_id());
    report.lot_type = req.lot_type;
    report.exec_type = EXEC_TYPE_TRADE;
    report.execution_price = req.price;
    if (traded == quantity) {
      report.order_status = 2;
      report.cumulative_quantity = req.order_quantity;
      report.leaves_quantity = 0;
      report.execution_quantity = this_traded * 100000000UL;
    } else {
      report.order_status = 1;
      report.cumulative_quantity = traded * 100000000UL;
      report.leaves_quantity = req.order_quantity - report.cumulative_quantity;
      report.execution_quantity = this_traded * 100000000UL;
    }

    reports.emplace_back(report);
  }

  return reports;
}
