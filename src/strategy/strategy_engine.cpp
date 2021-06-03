// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include <dlfcn.h>

#include <fstream>

#include "ft/base/contract_table.h"
#include "ft/base/log.h"
#include "ft/strategy/strategy.h"
#include "ft/utils/getopt.hpp"

static void Usage() {
  printf("Usage: ./strategy_engine <--config=file> [-h -? --help]\n");
  printf("                         <--name=name> [--loglevel=level]\n");
  printf("                         <--strategy=so>\n");
  printf("\n");
  printf("    --config            配置文件\n");
  printf("    -h, -?, --help      帮助\n");
  printf("    --name              策略的唯一标识，用于接收订单回报\n");
  printf("    --loglevel          日志等级(trace, debug, info, warn, error)\n");
  printf("    --strategy          要加载的策略的动态库\n");
}

int main() {
  std::string config_file = getarg("", "--config");
  std::string strategy_file = getarg("", "--strategy");
  std::string log_level = getarg("info", "--loglevel");
  std::string strategy_id = getarg("strategy", "--name");
  bool backtest_mode = getarg(false, "--backtest");
  bool help = getarg(false, "-h", "--help", "-?");

  if (help) {
    Usage();
    exit(0);
  }

  spdlog::set_level(spdlog::level::from_str(log_level));

  ft::FlareTraderConfig config;
  if (!config.Load(config_file)) {
    LOG_ERROR("failed to load config file {}", config_file);
    return false;
  }

  void* handle = dlopen(strategy_file.c_str(), RTLD_LAZY);
  if (!handle) {
    LOG_ERROR("Invalid strategy .so");
    exit(EXIT_FAILURE);
  }

  auto strategy_ctor = reinterpret_cast<ft::StrategyRunner* (*)()>(dlsym(handle, "CreateStrategy"));
  if (!strategy_ctor) {
    char* error_str = dlerror();
    if (error_str) {
      LOG_ERROR("CreateStrategy not found. error: {}", error_str);
      exit(EXIT_FAILURE);
    }
  }

  auto strategy = strategy_ctor();
  for (auto& strategy_conf : config.strategy_config_list) {
    if (strategy_conf.strategy_name == strategy_id) {
      if (!strategy->Init(strategy_conf, config)) {
        LOG_ERROR("failed to init strategy");
        exit(EXIT_FAILURE);
      }

      spdlog::info("ready to start strategy. name={}", strategy_id);
      if (backtest_mode) {
        strategy->RunBacktest();
      } else {
        strategy->Run();
      }
    }
  }

  LOG_ERROR("strategy config not found. strategy name: {}", strategy_id);
  exit(EXIT_FAILURE);
}
