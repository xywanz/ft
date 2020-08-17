// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_INCLUDE_CEP_DATA_CONFIG_H_
#define FT_INCLUDE_CEP_DATA_CONFIG_H_

#include <spdlog/spdlog.h>

#include <string>
#include <vector>

namespace ft {

class Config {
 public:
  std::string api{""};
  std::string trade_server_address{""};
  std::string quote_server_address{""};
  std::string broker_id{""};
  std::string investor_id{""};
  std::string password{""};
  std::string auth_code{""};
  std::string app_id{""};
  std::vector<std::string> subscription_list{};

  std::string contracts_file{""};

  bool no_receipt_mode = false;
  bool cancel_outstanding_orders_on_startup = true;

  uint64_t throttle_rate_limit_period_ms = 0;
  uint64_t throttle_rate_order_limit = 0;
  uint64_t throttle_rate_volume_limit = 0;

  int key_of_cmd_queue = 0;  // <= 0 means not to use order queue

  std::string arg0{""};
  std::string arg1{""};
  std::string arg2{""};
  std::string arg3{""};
  std::string arg4{""};
  std::string arg5{""};
  std::string arg6{""};
  std::string arg7{""};
  std::string arg8{""};

 public:
  void show() const {
    spdlog::info("Config:");
    spdlog::info("  trade_server_address: {}", trade_server_address);
    spdlog::info("  quote_server_address: {}", quote_server_address);
    spdlog::info("  broker_id: {}", broker_id);
    spdlog::info("  investor_id: {}", investor_id);
    spdlog::info("  password: ******");
    spdlog::info("  auth_code: {}", auth_code);
    spdlog::info("  app_id: {}", app_id);
    spdlog::info("  subscription_list: ");
    for (const auto& ticker : subscription_list) printf("%s ", ticker.c_str());
    printf("\n");
    spdlog::info("  contracts_file: {}", contracts_file.c_str());
    spdlog::info("  cancel_outstanding_orders_on_startup: {}",
                 cancel_outstanding_orders_on_startup);
    if (!arg0.empty()) spdlog::info("  arg0: {}", arg0);
    if (!arg1.empty()) spdlog::info("  arg1: {}", arg1);
    if (!arg2.empty()) spdlog::info("  arg2: {}", arg2);
    if (!arg3.empty()) spdlog::info("  arg3: {}", arg3);
    if (!arg4.empty()) spdlog::info("  arg4: {}", arg4);
    if (!arg5.empty()) spdlog::info("  arg5: {}", arg5);
    if (!arg6.empty()) spdlog::info("  arg6: {}", arg6);
    if (!arg7.empty()) spdlog::info("  arg7: {}", arg7);
    if (!arg8.empty()) spdlog::info("  arg8: {}", arg8);
  }
};

}  // namespace ft

#endif  // FT_INCLUDE_CEP_DATA_CONFIG_H_
