// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include <dlfcn.h>
#include <spdlog/spdlog.h>

#include <fstream>
#include <getopt.hpp>

#include "Core/ContractTable.h"
#include "Strategy/Strategy.h"
#include "TestCommon.h"

int main() {
  std::string contracts_file =
      getarg("../config/contracts.csv", "--contracts-file");
  std::string strategy_file = getarg("", "--strategy");
  std::string log_level = getarg("info", "--loglevel");
  std::string strategy_id = getarg("Strategy", "id");

  spdlog::set_level(spdlog::level::from_str(log_level));

  if (!ft::ContractTable::init(contracts_file)) {
    spdlog::error("Invalid file of contract list");
    exit(-1);
  }

  void* handle = dlopen(strategy_file.c_str(), RTLD_LAZY);
  if (!handle) {
    spdlog::error("Invalid strategy .so");
    exit(-1);
  }

  char* error;
  auto create_strategy =
      reinterpret_cast<ft::Strategy* (*)()>(dlsym(handle, "create_strategy"));
  if ((error = dlerror()) != nullptr) {
    spdlog::error("create_strategy not found. error: {}", error);
    exit(-1);
  }

  auto strategy = create_strategy();
  strategy->set_id(strategy_id);
  strategy->run();
}
