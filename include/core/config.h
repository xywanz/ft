// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_INCLUDE_CORE_CONFIG_H_
#define FT_INCLUDE_CORE_CONFIG_H_

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
    printf("Config:\n");
    printf("  trade_server_address: %s\n", trade_server_address.c_str());
    printf("  quote_server_address: %s\n", quote_server_address.c_str());
    printf("  broker_id: %s\n", broker_id.c_str());
    printf("  investor_id: %s\n", investor_id.c_str());
    printf("  password: ******\n");
    printf("  auth_code: %s\n", auth_code.c_str());
    printf("  app_id: %s\n", app_id.c_str());
    printf("  subscription_list: ");
    for (const auto& ticker : subscription_list) printf("%s ", ticker.c_str());
    printf("\n");
    printf("  cancel_outstanding_orders_on_startup: %s\n",
           cancel_outstanding_orders_on_startup ? "true" : "false");
    if (!arg0.empty()) printf("  arg0: %s\n", arg0.c_str());
    if (!arg1.empty()) printf("  arg1: %s\n", arg1.c_str());
    if (!arg2.empty()) printf("  arg2: %s\n", arg2.c_str());
    if (!arg3.empty()) printf("  arg3: %s\n", arg3.c_str());
    if (!arg4.empty()) printf("  arg4: %s\n", arg4.c_str());
    if (!arg5.empty()) printf("  arg5: %s\n", arg5.c_str());
    if (!arg6.empty()) printf("  arg6: %s\n", arg6.c_str());
    if (!arg7.empty()) printf("  arg7: %s\n", arg7.c_str());
    if (!arg8.empty()) printf("  arg8: %s\n", arg8.c_str());
  }
};

}  // namespace ft

#endif  // FT_INCLUDE_CORE_CONFIG_H_
