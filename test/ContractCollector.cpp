// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include <atomic>
#include <iostream>

#include <getopt.hpp>
#include <spdlog/spdlog.h>

#include "Api/Ctp/CtpApi.h"
#include "Base/DataStruct.h"
#include "Base/EventEngine.h"
#include "TestCommon.h"
#include "TradingManagement/ContractTable.h"

class ContractCollector {
 public:
  ContractCollector()
    : engine_(new ft::EventEngine) {
    api_ = new ft::CtpApi(engine_);
    engine_->set_handler(ft::EV_CONTRACT, MEM_HANDLER(ContractCollector::on_contract));

    engine_->run(false);
  }

  bool login(const ft::LoginParams& params) {
    if (!api_->login(params)) {
      spdlog::error("[TradeInfoCollector] login. Failed to login into trading server");
      return false;
    }
    is_login_ = true;
    return true;
  }

  bool dump_contracts(const std::string& file) {
    if (!is_login_)
      return false;

    if (!api_->query_contracts())
      return false;

    engine_->stop();

    store_contracts(file, contracts_);
    return true;
  }

  void on_contract(cppex::Any* data) {
    auto* contract = data->cast<ft::Contract>();
    contracts_.emplace_back(std::move(*contract));
  }

 private:
  ft::EventEngine* engine_;
  ft::GeneralApi* api_;
  std::vector<ft::Contract> contracts_;
  std::atomic<bool> is_login_ = false;
};


int main() {
  std::string login_config_file = getarg("../config/login.yaml", "--login-config");
  std::string path = getarg("../config/contracts.csv", "--output-path");

  ft::LoginParams params;
  if (!load_login_params(login_config_file, &params)) {
    spdlog::error("Invalid file of login config");
    exit(-1);
  }
  params.set_md_server_addr("");

  auto* collector = new ContractCollector;
  if (!collector->login(params)) {
    exit(-1);
  }

  if (!collector->dump_contracts(path)) {
    spdlog::error("Failed to export contracts");
    exit(-1);
  }

  spdlog::info("Successfully exported contracts");
}
