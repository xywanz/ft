// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include <dlfcn.h>

#include <getopt.hpp>

#include "ft/base/contract_table.h"
#include "ft/trader/gateway.h"
#include "ft/utils/config_loader.h"
#include "spdlog/spdlog.h"

static void Usage() {
  printf("Usage:\n");
  printf("    --config            登录的配置文件\n");
  printf("    -h, -?, --help      帮助\n");
  printf("    --output            生成的合约路径及文件名，如./contracts.csv\n");
}

int main() {
  std::string login_yml = getarg("../config/config.yml", "--config");
  std::string output = getarg("./contracts.csv", "--output");
  std::string loglevel = getarg("info", "--loglevel");
  bool help = getarg(false, "-h", "--help", "-?");

  if (help) {
    Usage();
    exit(0);
  }

  spdlog::set_level(spdlog::level::from_str(loglevel));

  ft::Config config;
  ft::LoadConfig(login_yml, &config);

  void* handle = dlopen(config.api.c_str(), RTLD_LAZY);
  auto gateway_ctor = reinterpret_cast<ft::GatewayCreateFunc>(dlsym(handle, "CreateGateway"));
  if (!gateway_ctor) {
    spdlog::error("symbol CreateGateway not found");
    exit(EXIT_FAILURE);
  }
  auto* gateway = gateway_ctor();
  if (!gateway) {
    spdlog::error("failed to create gateway");
    exit(EXIT_FAILURE);
  }
  if (!gateway->Init(config)) {
    spdlog::error("failed to init gateway");
    exit(EXIT_FAILURE);
  }

  if (!gateway->QueryContracts()) {
    spdlog::error("failed to query contracts");
    exit(EXIT_FAILURE);
  }

  auto* rb = gateway->GetQryResultRB();
  ft::GatewayQueryResult res;

  std::vector<ft::Contract> contracts;
  for (;;) {
    rb->GetWithBlocking(&res);
    if (res.msg_type == ft::GatewayMsgType::kContractEnd) {
      break;
    }
    if (res.msg_type != ft::GatewayMsgType::kContract) {
      abort();
    }
    contracts.emplace_back(std::get<ft::Contract>(res.data));
  }

  ft::StoreContractList(output, contracts);

  spdlog::info("successfully dump to {}", output);
  exit(EXIT_SUCCESS);
}
