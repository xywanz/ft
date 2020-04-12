// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include <atomic>
#include <iostream>

#include <spdlog/spdlog.h>

#include "ctp/CtpGateway.h"
#include "TraderInterface.h"

const char* kSimnowTradeAddr[] = {
  "tcp://180.168.146.187:10130",
  "tcp://180.168.146.187:10100",
  "tcp://180.168.146.187:10101",
  "tcp://218.202.237.33:10102"
};

const char* kBrokerID = "9999";
const char* kInvestorID = "122899";
const char* kPasswd = "lsk4129691";
const char* kAuthCode = "0000000000000000";
const char* kAppID = "simnow_client_test";

class ContractExporter : public ft::TraderInterface {
 public:
  ContractExporter() {
    gateway_ = new ft::CtpGateway;
    gateway_->register_cb(this);
  }

  bool login(const ft::LoginParams& params) {
    if (!gateway_->login(params)) {
      spdlog::error("[ContractExporter] login. Failed to login into trading server");
      return false;
    }
    is_login_ = true;
    return true;
  }

  bool dump_contracts(const std::string& file) {
    if (!is_login_)
      return false;

    auto status = gateway_->query_contract("", "");
    if (!status.wait())
      return false;

    std::ofstream ofs(file, std::ios_base::trunc);
    std::string line = fmt::format("#ticker,size,price_tick\n");
    ofs << line;
    for (const auto& [ticker, contract] : contracts_) {
      line = fmt::format("{},{},{}\n",
                         contract.ticker,
                         contract.size,
                         contract.price_tick);
      ofs << line;
    }

    ofs.close();
    return true;
  }

  void on_contract(const ft::Contract* contract) {
    contracts_[contract->ticker] = *contract;
  }

 private:
  ft::GatewayInterface* gateway_;
  std::map<std::string, ft::Contract> contracts_;
  std::atomic<bool> is_login_ = false;
};

int main(int argc, char** argv) {
  spdlog::set_level(spdlog::level::debug);

  auto exporter = new ContractExporter;
  ft::LoginParams params;
  bool status;

  if (argc == 2) {
    int i = std::stoi(argv[1]);
    if (i < 0 || i > 3)
      exit(-1);
    params.set_front_addr(kSimnowTradeAddr[i]);
  } else {
    params.set_front_addr(kSimnowTradeAddr[0]);
  }

  params.set_broker_id(kBrokerID);
  params.set_investor_id(kInvestorID);
  params.set_passwd(kPasswd);
  params.set_auth_code(kAuthCode);
  params.set_app_id(kAppID);

  if (!exporter->login(params)) {
    exit(-1);
  }

  if (!exporter->dump_contracts("contracts.txt")) {
    spdlog::error("Failed to export contracts");
    exit(-1);
  }

  spdlog::info("Successfully exported contracts");
}
