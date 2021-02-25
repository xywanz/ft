// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include <dlfcn.h>
#include <ft/base/contract_table.h>
#include <ft/trader/gateway.h>
#include <ft/utils/config_loader.h>
#include <spdlog/spdlog.h>

#include <getopt.hpp>

class ContractCollector : public ft::BaseOrderManagementSystem {
 public:
  bool Login(const ft::Config& config) {
    void* handle = dlopen(config.api.c_str(), RTLD_LAZY);
    auto gateway_ctor = reinterpret_cast<ft::GatewayCreateFunc>(dlsym(handle, "CreateGateway"));
    if (!gateway_ctor) {
      return false;
    }
    gateway_.reset(gateway_ctor());
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
  ft::LoadConfig(login_yml, &config);
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
