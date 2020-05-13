// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_TRADINGSYSTEM_CONFIG_H_
#define FT_TRADINGSYSTEM_CONFIG_H_

#include <yaml-cpp/yaml.h>

#include <fstream>
#include <string>

#include "Core/LoginParams.h"

inline bool load_login_params(const std::string& file,
                              ft::LoginParams* params) {
  std::ifstream ifs(file);
  if (!ifs) return false;

  YAML::Node config = YAML::LoadFile(file);
  params->set_api(config["api"].as<std::string>());
  params->set_front_addr(config["front_addr"].as<std::string>());
  params->set_md_server_addr(config["md_server_addr"].as<std::string>());
  params->set_broker_id(config["broker_id"].as<std::string>());
  params->set_investor_id(config["investor_id"].as<std::string>());
  params->set_passwd(config["passwd"].as<std::string>());
  params->set_auth_code(config["auth_code"].as<std::string>());
  params->set_app_id(config["app_id"].as<std::string>());
  params->set_subscribed_list({config["ticker"].as<std::string>()});

  return true;
}

#endif  // FT_TRADINGSYSTEM_CONFIG_H_
