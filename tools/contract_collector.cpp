// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include <dlfcn.h>

#include "ft/base/contract_table.h"
#include "ft/utils/getopt.hpp"
#include "spdlog/spdlog.h"
#include "trader/gateway/gateway.h"

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

  ft::FlareTraderConfig config;
  config.Load(login_yml);

  auto gateway = ft::CreateGateway(config.gateway_config.api);
  if (!gateway) {
    spdlog::error("failed to create gateway");
    exit(EXIT_FAILURE);
  }
  if (!gateway->Init(config.gateway_config)) {
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
