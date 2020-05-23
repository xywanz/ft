// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_SRC_TRADINGSYSTEM_CONFIGLOADER_H_
#define FT_SRC_TRADINGSYSTEM_CONFIGLOADER_H_

#include <yaml-cpp/yaml.h>

#include <string>
#include <vector>

#include "Core/Config.h"

namespace ft {

inline void split(const std::string_view& str, const std::string_view& delim,
                  std::vector<std::string>* results) {
  std::size_t start = 0, end, size;
  while ((end = str.find(delim, start)) != std::string::npos) {
    size = end - start;
    if (size != 0)
      results->emplace_back(std::string(str.cbegin() + start, size));
    start = end + delim.size();
  }

  if (start != str.size())
    results->emplace_back(
        std::string(str.cbegin() + start, str.size() - start));
}

inline void load_config(const std::string& file, ft::Config* config) {
  std::ifstream ifs(file);
  assert(ifs);

  YAML::Node node = YAML::LoadFile(file);

  config->api = node["api"].as<std::string>("");
  config->trade_server_address =
      node["trade_server_address"].as<std::string>("");
  config->quote_server_address =
      node["quote_server_address"].as<std::string>("");
  config->broker_id = node["broker_id"].as<std::string>("");
  config->investor_id = node["investor_id"].as<std::string>("");
  config->password = node["password"].as<std::string>("");
  config->auth_code = node["auth_code"].as<std::string>("");
  config->app_id = node["app_id"].as<std::string>("");
  config->arg0 = node["arg0"].as<std::string>("");
  config->arg1 = node["arg1"].as<std::string>("");
  config->arg2 = node["arg2"].as<std::string>("");
  config->arg3 = node["arg3"].as<std::string>("");
  config->arg4 = node["arg4"].as<std::string>("");
  config->arg5 = node["arg5"].as<std::string>("");
  config->arg6 = node["arg6"].as<std::string>("");
  config->arg7 = node["arg7"].as<std::string>("");
  config->arg8 = node["arg8"].as<std::string>("");

  auto subs = node["subscription_list"].as<std::string>("");
  if (!subs.empty()) {
    std::vector<std::string> sub_list;
    split(subs, ",", &config->subscription_list);
  }
}

}  // namespace ft

#endif  // FT_SRC_TRADINGSYSTEM_CONFIGLOADER_H_
