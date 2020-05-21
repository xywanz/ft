// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include <spdlog/spdlog.h>

#include <getopt.hpp>

#include "Core/ContractTable.h"
#include "Core/Gateway.h"
#include "TradingSystem/Config.h"

class ContractCollector : public ft::TradingEngineInterface {
 public:
  bool login(const ft::LoginParams& params) {
    gateway_.reset(ft::create_gateway(params.api(), this));
    if (!gateway_) return false;

    return gateway_->login(params);
  }

  bool dump(const std::string& file = "./contracts.csv") {
    if (!gateway_->query_contracts()) return false;
    ft::store_contracts(file, contracts_);
    return true;
  }

  void on_query_contract(const ft::Contract* contract) override {
    contracts_.emplace_back(*contract);
  }

 private:
  std::unique_ptr<ft::Gateway> gateway_;
  std::vector<ft::Contract> contracts_;
};

int main() {
  std::string login_yml = getarg("../config/login.yml", "--config");
  std::string output = getarg("./contracts.csv", "--output");
  std::string loglevel = getarg("info", "--loglevel");

  spdlog::set_level(spdlog::level::from_str(loglevel));

  ContractCollector collector;
  ft::LoginParams params;
  load_login_params(login_yml, &params);
  if (!collector.login(params)) {
    printf("failed to login\n");
    exit(-1);
  }

  if (!collector.dump(output)) {
    printf("failed to dump\n");
  }

  printf("successfully dump to %s\n", output.c_str());
  exit(0);
}
