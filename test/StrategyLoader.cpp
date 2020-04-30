// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include <dlfcn.h>
#include <fstream>

#include <getopt.hpp>
#include <spdlog/spdlog.h>

#include "AlgoTrade/StrategyEngine.h"
#include "Base/DataStruct.h"
#include "TestCommon.h"


int main() {
  std::string login_config_file = getarg("../config/login.yaml", "--login-config");
  std::string contracts_file = getarg("../config/contracts.csv", "--contracts-file");
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

  ft::StrategyEngine engine(ft::FrontType::CTP);

  engine.login(params);

  void* handle = dlopen(strategy_file.c_str(), RTLD_LAZY);
  if (!handle) {
    spdlog::error("Invalid strategy .so");
    exit(-1);
  }

  char* error;
  auto create_strategy = reinterpret_cast<void*(*)()>(dlsym(handle, "create_strategy"));
  if ((error = dlerror()) != nullptr) {
    spdlog::error("create_strategy not found. error: {}", error);
    exit(-1);
  }

  auto strategy = reinterpret_cast<ft::Strategy*>(create_strategy());
  engine.mount_strategy(params.subscribed_list()[0], strategy);

  while (1) {
    std::this_thread::sleep_for(std::chrono::seconds(60));
  }
}
