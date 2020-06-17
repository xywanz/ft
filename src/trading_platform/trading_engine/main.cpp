// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include <spdlog/spdlog.h>

#include <getopt.hpp>

#include "core/contract_table.h"
#include "trading_engine/config_loader.h"
#include "trading_engine/trading_engine.h"

ft::TradingEngine* engine = nullptr;

static void usage() {
  printf("usage: ./trading-engine [--config=<file>] [--contracts=<file>]\n");
  printf("                        [-h -? --help] [--loglevel=level]\n");
  printf("\n");
  printf("    --config            登录的配置文件\n");
  printf("    --contracts         合约列表文件\n");
  printf("    -h, -?, --help      帮助\n");
  printf("    --loglevel          日志等级(info, warn, error, debug, trace)\n");
}

int main() {
  std::string login_config_file =
      getarg("../config/ctp_config.yml", "--config");
  std::string contracts_file = getarg("../config/contracts.csv", "--contracts");
  std::string log_level = getarg("info", "--loglevel");
  bool help = getarg(false, "-h", "--help", "-?");

  if (help) {
    usage();
    exit(0);
  }

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
