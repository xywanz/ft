// Copyright [2020-present] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_INCLUDE_FT_BASE_CONFIG_H_
#define FT_INCLUDE_FT_BASE_CONFIG_H_

#include <map>
#include <string>
#include <vector>

namespace ft {

struct GlobalConfig {
  std::string contract_file;
  std::string trader_db_address;
};

struct GatewayConfig {
  std::string api;
  std::string trade_server_address;
  std::string quote_server_address;
  std::string broker_id;
  std::string investor_id;
  std::string password;
  std::string auth_code;
  std::string app_id;
  std::vector<std::string> subscription_list;
  bool cancel_outstanding_orders_on_startup;

  std::map<std::string, std::string> extended_args;
};

struct RiskConfig {
  std::string name;
  std::map<std::string, std::string> options;
};

struct RmsConfig {
  std::vector<RiskConfig> risk_conf_list;
};

struct StrategyConfig {
  std::string strategy_name;
  std::string trade_mq_name;
  std::string rsp_mq_name;
  std::string md_mq_name;
  std::vector<std::string> subscription_list;
};

struct FlareTraderConfig {
  bool Load(const std::string& file);

  GlobalConfig global_config;
  GatewayConfig gateway_config;
  RmsConfig rms_config;
  std::vector<StrategyConfig> strategy_config_list;
};

}  // namespace ft

#endif  // FT_INCLUDE_FT_BASE_CONFIG_H_
