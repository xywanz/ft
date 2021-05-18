// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include <dlfcn.h>

#include <fstream>
#include <getopt.hpp>
#include <thread>

#include "ft/base/contract_table.h"
#include "ft/component/pubsub/subscriber.h"
#include "ft/strategy/strategy.h"
#include "spdlog/spdlog.h"

static void Usage() {
  printf("Usage: ./strategy_engine [--account=<account>]\n");
  printf("                         [--contracts=<file>] [-h -? --help]\n");
  printf("                         [--id=<id>] [--loglevel=level]\n");
  printf("                         [--strategy=<so>]\n");
  printf("\n");
  printf("    --account           账户\n");
  printf("    --contracts         合约列表文件\n");
  printf("    -h, -?, --help      帮助\n");
  printf("    --id                策略的唯一标识，用于接收订单回报\n");
  printf("    --loglevel          日志等级(trace, debug, info, warn, error)\n");
  printf("    --strategy          要加载的策略的动态库\n");
}

int main() {
  std::string config_file = getarg("", "--config");
  std::string strategy_file = getarg("", "--strategy");
  std::string log_level = getarg("info", "--loglevel");
  std::string strategy_id = getarg("strategy", "--name");
  uint64_t account_id = getarg(0ULL, "--account");
  bool backtest_mode = getarg(false, "--backtest");
  bool help = getarg(false, "-h", "--help", "-?");

  if (help) {
    Usage();
    exit(0);
  }

  spdlog::set_level(spdlog::level::from_str(log_level));

  if (account_id == 0) {
    spdlog::error("Please input account id");
    exit(EXIT_FAILURE);
  }

  ft::FlareTraderConfig config;
  if (!config.Load(config_file)) {
    spdlog::error("failed to load config file {}", config_file);
    return false;
  }

  if (!ft::ContractTable::Init(config.global_config.contract_file)) {
    spdlog::error("Invalid file of contract list");
    exit(EXIT_FAILURE);
  }

  void* handle = dlopen(strategy_file.c_str(), RTLD_LAZY);
  if (!handle) {
    spdlog::error("Invalid strategy .so");
    exit(EXIT_FAILURE);
  }

  auto strategy_ctor = reinterpret_cast<ft::Strategy* (*)()>(dlsym(handle, "CreateStrategy"));
  if (!strategy_ctor) {
    char* error_str = dlerror();
    if (error_str) {
      spdlog::error("CreateStrategy not found. error: {}", error_str);
      exit(EXIT_FAILURE);
    }
  }

  auto strategy = strategy_ctor();
  for (auto& strategy_conf : config.strategy_config_list) {
    if (strategy_conf.strategy_name == strategy_id) {
      if (!strategy->Init(strategy_conf)) {
        spdlog::error("failed to init strategy");
        exit(EXIT_FAILURE);
      }
      strategy->SetStrategyId(strategy_id);
      strategy->SetAccountId(account_id);
      strategy->SetBacktestMode(backtest_mode);

      spdlog::info("ready to start strategy. id={}, account={}", strategy_id, account_id);
      strategy->Run();
    }
  }

  spdlog::error("strategy config not found. strategy name: {}", strategy_id);
  exit(EXIT_FAILURE);
}
