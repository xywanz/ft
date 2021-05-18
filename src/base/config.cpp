// Copyright [2020-2021] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "ft/base/config.h"

#include "spdlog/spdlog.h"
#include "yaml-cpp/yaml.h"

namespace ft {

bool FlareTraderConfig::Load(const std::string& file) {
  try {
    YAML::Node node = YAML::LoadFile(file);

    auto global_item = node["global"];
    global_config.contract_file = global_item["contract_file"].as<std::string>("");

    auto gateway_item = node["gateway_config"];
    gateway_config.api = gateway_item["api"].as<std::string>();
    gateway_config.trade_server_address = gateway_item["trade_server_address"].as<std::string>("");
    gateway_config.quote_server_address = gateway_item["quote_server_address"].as<std::string>("");
    gateway_config.broker_id = gateway_item["broker_id"].as<std::string>("");
    gateway_config.investor_id = gateway_item["investor_id"].as<std::string>("");
    gateway_config.password = gateway_item["password"].as<std::string>("");
    gateway_config.auth_code = gateway_item["auth_code"].as<std::string>("");
    gateway_config.app_id = gateway_item["app_id"].as<std::string>("");
    gateway_config.arg0 = gateway_item["arg0"].as<std::string>("");
    gateway_config.arg1 = gateway_item["arg1"].as<std::string>("");
    gateway_config.arg2 = gateway_item["arg2"].as<std::string>("");
    gateway_config.arg3 = gateway_item["arg3"].as<std::string>("");
    gateway_config.cancel_outstanding_orders_on_startup =
        gateway_item["cancel_outstanding_orders_on_startup"].as<bool>(true);

    auto rms_item = node["rms_config"];
    rms_config.throttle_rate_limit_period_ms =
        rms_item["throttle_rate_limit_period_ms"].as<uint64_t>(0);
    rms_config.throttle_rate_order_limit = rms_item["throttle_rate_order_limit"].as<uint64_t>(0);
    rms_config.throttle_rate_volume_limit = rms_item["throttle_rate_volume_limit"].as<uint64_t>(0);

    auto strategy_item_list = node["strategy_list"];
    for (auto strategy_item : strategy_item_list) {
      StrategyConfig strategy_config;
      strategy_config.strategy_name = strategy_item["name"].as<std::string>();
      strategy_config.trade_mq_name = strategy_item["trade_mq"].as<std::string>();
      strategy_config.rsp_mq_name = strategy_item["rsp_mq"].as<std::string>();
      strategy_config.md_mq_name = strategy_item["md_mq"].as<std::string>();
      strategy_config.subscription_list =
          strategy_item["subscription_list"].as<std::vector<std::string>>(
              std::vector<std::string>{});
      strategy_config_list.emplace_back(std::move(strategy_config));
    }

    return true;
  } catch (YAML::Exception& e) {
    spdlog::error("{}", e.what());
    return false;
  } catch (std::exception& e) {
    spdlog::error("{}", e.what());
    return false;
  } catch (...) {
    return false;
  }
}

}  // namespace ft
