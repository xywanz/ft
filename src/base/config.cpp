// Copyright [2020-present] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "ft/base/config.h"

#include "ft/base/log.h"
#include "yaml-cpp/yaml.h"

namespace ft {

bool FlareTraderConfig::Load(const std::string& file) {
  try {
    YAML::Node node = YAML::LoadFile(file);

    auto global_item = node["global"];
    global_config.contract_file = global_item["contract_file"].as<std::string>("");
    global_config.trader_db_address = global_item["trader_db_address"].as<std::string>("");

    auto gateway_item = node["gateway"];
    gateway_config.api = gateway_item["api"].as<std::string>();
    gateway_config.trade_server_address = gateway_item["trade_server_address"].as<std::string>("");
    gateway_config.quote_server_address = gateway_item["quote_server_address"].as<std::string>("");
    gateway_config.broker_id = gateway_item["broker_id"].as<std::string>("");
    gateway_config.investor_id = gateway_item["investor_id"].as<std::string>("");
    gateway_config.password = gateway_item["password"].as<std::string>("");
    gateway_config.auth_code = gateway_item["auth_code"].as<std::string>("");
    gateway_config.app_id = gateway_item["app_id"].as<std::string>("");
    gateway_config.cancel_outstanding_orders_on_startup =
        gateway_item["cancel_outstanding_orders_on_startup"].as<bool>(true);

    auto extended_args = gateway_item["extended_args"];
    for (auto it = extended_args.begin(); it != extended_args.end(); ++it) {
      auto key = it->first.as<std::string>();
      auto val = it->second.as<std::string>();
      gateway_config.extended_args.emplace(key, val);
    }

    auto rms_item = node["rms"];
    assert(rms_item.IsSequence());
    for (std::size_t i = 0; i < rms_item.size(); ++i) {
      RiskConfig risk_conf{};
      auto risk = rms_item[i];
      assert(risk.IsMap());

      risk_conf.name = risk["name"].as<std::string>();
      for (auto it = risk.begin(); it != risk.end(); ++it) {
        auto key = it->first.as<std::string>();
        if (key != "name") {
          auto value = it->second.as<std::string>();
          risk_conf.options.emplace(key, value);
        }
      }
      rms_config.risk_conf_list.emplace_back(std::move(risk_conf));
    }

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
    LOG_ERROR("{}", e.what());
    return false;
  } catch (std::exception& e) {
    LOG_ERROR("{}", e.what());
    return false;
  } catch (...) {
    return false;
  }
}

}  // namespace ft
