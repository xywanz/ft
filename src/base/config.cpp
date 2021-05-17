// Copyright [2020-2021] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "ft/base/config.h"

#include "spdlog/spdlog.h"
#include "yaml-cpp/yaml.h"

namespace ft {

bool FlareTraderConfig::Load(const std::string& file) {
  try {
    YAML::Node node = YAML::LoadFile(file);

    auto oms_item = node["oms_config"];
    oms_config.contract_file = oms_item["contract_file"].as<std::string>("");

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

    auto rms_item = node["rms_config"];
    rms_config.no_receipt_mode = rms_item["no_receipt_mode"].as<bool>(true);
    rms_config.throttle_rate_limit_period_ms =
        rms_item["throttle_rate_limit_period_ms"].as<uint64_t>(0);
    rms_config.throttle_rate_order_limit = rms_item["throttle_rate_order_limit"].as<uint64_t>(0);
    rms_config.throttle_rate_volume_limit = rms_item["throttle_rate_volume_limit"].as<uint64_t>(0);
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
