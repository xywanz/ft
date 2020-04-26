// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_TEST_TESTCOMMON_H_
#define FT_TEST_TESTCOMMON_H_

#include <fstream>
#include <string>

#include <yaml-cpp/yaml.h>

#include "Base/DataStruct.h"

inline bool load_login_params(const std::string& file, ft::LoginParams* params) {
  std::ifstream ifs(file);
  if (!ifs)
    return false;

  YAML::Node config = YAML::LoadFile(file);
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

#endif  // FT_TEST_TESTCOMMON_H_
