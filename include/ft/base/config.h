// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_INCLUDE_FT_BASE_CONFIG_H_
#define FT_INCLUDE_FT_BASE_CONFIG_H_

#include <string>
#include <vector>

namespace ft {

struct Config {
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
};

}  // namespace ft

#endif  // FT_INCLUDE_FT_BASE_CONFIG_H_
