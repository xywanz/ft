// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include <spdlog/spdlog.h>

#include <getopt.hpp>

#include "gateway/gateway.h"
#include "trading_server/datastruct/contract_table.h"
#include "trading_server/order_management/config_loader.h"

class ContractCollector : public ft::BaseOrderManagementSystem {
 public:
  bool Login(const ft::Config& config) {
    gateway_.reset(ft::CreateGateway(config.api));
    if (!gateway_) return false;

    return gateway_->Login(this, config);
  }

  bool Dump(const std::string& file = "./contracts.csv") {
    if (!gateway_->QueryContractList(&contracts_)) return false;
    ft::StoreContractList(file, contracts_);
    return true;
  }

 private:
  std::unique_ptr<ft::Gateway> gateway_;
  std::vector<ft::Contract> contracts_;
};

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

  ContractCollector collector;
  ft::Config config;
  ft::load_config(login_yml, &config);
  if (!collector.Login(config)) {
    printf("failed to Login\n");
    exit(-1);
  }

  if (!collector.Dump(output)) {
    printf("failed to dump\n");
  }

  printf("successfully dump to %s\n", output.c_str());
  exit(0);
}
