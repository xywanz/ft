// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include <spdlog/spdlog.h>

#include <getopt.hpp>

#include "Core/ContractTable.h"
#include "TradingSystem/Config.h"
#include "TradingSystem/TradingEngine.h"

ft::TradingEngine* engine = nullptr;

int main() {
  std::string login_config_file =
      getarg("../config/login.yml", "--login-config");
  std::string contracts_file =
      getarg("../config/contracts.csv", "--contracts-file");
  std::string strategy_file = getarg("", "--strategy");
  std::string log_level = getarg("info", "--loglevel");

  spdlog::set_level(spdlog::level::from_str(log_level));

  ft::LoginParams params;
  if (!load_login_params(login_config_file, &params)) {
    spdlog::error("Invalid file of login config");
    exit(-1);
  }

  if (!ft::ContractTable::init(contracts_file)) {
    spdlog::error("Invalid file of contract list");
    exit(-1);
  }

  engine = new ft::TradingEngine;

  if (!engine->login(params)) exit(-1);

  engine->run();
}
