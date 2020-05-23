// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include <spdlog/spdlog.h>

#include <getopt.hpp>

#include "Core/ContractTable.h"
#include "TradingSystem/ConfigLoader.h"
#include "TradingSystem/TradingEngine.h"

ft::TradingEngine* engine = nullptr;

int main() {
  std::string login_config_file =
      getarg("../config/ctp_config.yml", "--login-config");
  std::string contracts_file =
      getarg("../config/contracts.csv", "--contracts-file");
  std::string strategy_file = getarg("", "--strategy");
  std::string log_level = getarg("info", "--loglevel");

  spdlog::set_level(spdlog::level::from_str(log_level));

  ft::Config config;
  ft::load_config(login_config_file, &config);

  if (!ft::ContractTable::init(contracts_file)) {
    spdlog::error("Invalid file of contract list");
    exit(-1);
  }

  engine = new ft::TradingEngine;

  if (!engine->login(config)) exit(-1);

  engine->run();
}
