// Copyright [2020-2021] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "trading_server/config_loader.h"

namespace ft {

bool LoadConfig(const std::string& file, ft::Config* config) {
  std::ifstream ifs(file);
  if (!ifs) {
    return false;
  }

  YAML::Node node = YAML::LoadFile(file);

  config->api = node["api"].as<std::string>("");
  config->trade_server_address = node["trade_server_address"].as<std::string>("");
  config->quote_server_address = node["quote_server_address"].as<std::string>("");
  config->broker_id = node["broker_id"].as<std::string>("");
  config->investor_id = node["investor_id"].as<std::string>("");
  config->password = node["password"].as<std::string>("");
  config->auth_code = node["auth_code"].as<std::string>("");
  config->app_id = node["app_id"].as<std::string>("");

  config->subscription_list =
      node["subscription_list"].as<std::vector<std::string>>(std::vector<std::string>{});

  config->contracts_file = node["contracts_file"].as<std::string>("");

  config->no_receipt_mode = node["no_receipt_mode"].as<bool>(false);
  config->cancel_outstanding_orders_on_startup =
      node["cancel_outstanding_orders_on_startup"].as<bool>(true);

  config->throttle_rate_limit_period_ms = node["throttle_rate_limit_period_ms"].as<uint64_t>(0);
  config->throttle_rate_order_limit = node["throttle_rate_order_limit"].as<uint64_t>(0);
  config->throttle_rate_volume_limit = node["throttle_rate_volume_limit"].as<uint64_t>(0);

  config->key_of_cmd_queue = node["key_of_cmd_queue"].as<int>(0);

  config->arg0 = node["arg0"].as<std::string>("");
  config->arg1 = node["arg1"].as<std::string>("");
  config->arg2 = node["arg2"].as<std::string>("");
  config->arg3 = node["arg3"].as<std::string>("");
  config->arg4 = node["arg4"].as<std::string>("");
  config->arg5 = node["arg5"].as<std::string>("");
  config->arg6 = node["arg6"].as<std::string>("");
  config->arg7 = node["arg7"].as<std::string>("");
  config->arg8 = node["arg8"].as<std::string>("");

  return true;
}

}  // namespace ft
