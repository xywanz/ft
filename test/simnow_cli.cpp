// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include <iostream>

#include <ncurses.h>
#include <cppex/string.h>
#include <spdlog/spdlog.h>

#include "ctp/CtpMdReceiver.h"
#include "Trader.h"

const char* kSimnowTradeAddr[] = {
  "tcp://180.168.146.187:10130",
  "tcp://180.168.146.187:10100",
  "tcp://180.168.146.187:10101",
  "tcp://218.202.237.33:10102"
};

const char* kSimnowMdAddr[] = {
  "tcp://180.168.146.187:10131",
  "tcp://180.168.146.187:10110",
  "tcp://180.168.146.187:10111",
  "tcp://218.202.237.33:10112"
};

const char* kBrokerID = "9999";
const char* kInvestorID = "122899";
const char* kPasswd = "lsk4129691";
const char* kAuthCode = "0000000000000000";
const char* kAppID = "simnow_client_test";

void help() {
  fmt::print("help:\n");
  fmt::print("  query_position [ticker]\n");
  fmt::print("  query_account\n");
  fmt::print("  last_price\n");
  fmt::print("  buy_open <volume> <price>\n");
  fmt::print("  sell_open <volume> <price>\n");
  fmt::print("  buy_close <volume> <price>\n");
  fmt::print("  sell_close <volume> <price>\n");
}

int main(int argc, char** argv) {
  spdlog::set_level(spdlog::level::info);

  ft::ContractTable::init("./contracts.txt");
  ft::Trader trader(ft::FrontType::CTP);
  ft::LoginParams params;

  std::string ticker = "rb2009.SHFE";

  params.set_front_addr(kSimnowTradeAddr[0]);
  params.set_md_server_addr(kSimnowMdAddr[0]);

  params.set_broker_id(kBrokerID);
  params.set_investor_id(kInvestorID);
  params.set_passwd(kPasswd);
  params.set_auth_code(kAuthCode);
  params.set_app_id(kAppID);
  params.set_subscribed_list({ticker});

  if (!trader.login(params)) {
    exit(-1);
  }

  std::string cmd;
  std::vector<std::string> args;
  fmt::print(">>>");
  while (std::getline(std::cin, cmd)) {
    args.clear();
    split(cmd, " ", args);
    if (args.empty())
      goto next;

    if (args[0] == "help") {
      help();
    } else if (args[0] == "query_position") {
      trader.show_positions();
    } else if (args[0] == "query_account") {
      auto account = trader.get_account();
      spdlog::info("Account ID: {}, Balance: {}, Fronzen: {}",
                   account->account_id, account->balance, account->frozen);
    } else if (args[0] == "buy_open") {
      int volume = std::stoi(args[1]);
      double price = std::stod(args[2]);
      trader.buy_open(ticker, volume, ft::OrderType::FAK, price);
    } else if (args[0] == "sell_open") {
      int volume = std::stoi(args[1]);
      double price = std::stod(args[2]);
      trader.sell_open(ticker, volume, ft::OrderType::FAK, price);
    } else if (args[0] == "buy_close") {
      int volume = std::stoi(args[1]);
      double price = std::stod(args[2]);
      trader.buy_close(ticker, volume, ft::OrderType::FAK, price);
    } else if (args[0] == "sell_close") {
      int volume = std::stoi(args[1]);
      double price = std::stod(args[2]);
      trader.sell_close(ticker, volume, ft::OrderType::FAK, price);
    } else if (args[0] == "last_price") {
      auto kchart = trader.get_kchart(ticker, ft::Period::M1);
      if (kchart && !kchart->empty())
        fmt::print("last: {}\n", kchart->back().close);
    }

next:
    fmt::print(">>>");
  }

  trader.join();
}
