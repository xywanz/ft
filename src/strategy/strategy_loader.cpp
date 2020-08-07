// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include <dlfcn.h>
#include <spdlog/spdlog.h>

#include <fstream>
#include <getopt.hpp>

#include "cep/data/contract_table.h"
#include "strategy/strategy.h"

static void usage() {
  printf("usage: ./strategy-loader [--account=<account>] [--config=<file>]\n");
  printf("                         [--contracts=<file>] [-h -? --help]\n");
  printf("                         [--id=<id>] [--loglevel=level]\n");
  printf("                         [--strategy=<so>]\n");
  printf("\n");
  printf("    --account           账户\n");
  printf("    --contracts         合约列表文件\n");
  printf("    -h, -?, --help      帮助\n");
  printf("    --id                策略的唯一标识，用于接收订单回报\n");
  printf("    --loglevel          日志等级(info, warn, error, debug, trace)\n");
  printf("    --strategy          要加载的策略的动态库\n");
}

int main() {
  std::string contracts_file =
      getarg("../config/contracts.csv", "--contracts-file");
  std::string strategy_file = getarg("", "--strategy");
  std::string log_level = getarg("info", "--loglevel");
  std::string strategy_id = getarg("Strategy", "id");
  uint64_t account_id = getarg(0ULL, "--account");
  bool help = getarg(false, "-h", "--help", "-?");

  if (help) {
    usage();
    exit(0);
  }

  spdlog::set_level(spdlog::level::from_str(log_level));

  if (account_id == 0) {
    spdlog::error("Please input account id");
    exit(-1);
  }

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
  strategy->set_account_id(account_id);
  strategy->run();
}
