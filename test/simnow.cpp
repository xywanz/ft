// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include <iostream>

#include <getopt.hpp>
#include <spdlog/spdlog.h>

#include "ctp/CtpMdReceiver.h"
#include "Strategy.h"
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

class MyStrategy : public ft::Strategy {
 public:
  void on_init(ft::QuantitativTradingContext* ctx) override {
    spdlog::info("[MyStrategy::on_init]");
  }

  void on_tick(ft::QuantitativTradingContext* ctx) override {
    spdlog::info("[MyStrategy::on_tick]");
    ctx->buy_open(1, ft::OrderType::FOK, 3500);
  }
};

int main() {
  std::size_t front_index = getarg(0UL, "--front");
  int log_level = getarg(2, "--loglevel");

  spdlog::set_level(static_cast<spdlog::level::level_enum>(log_level));

  ft::Trader trader(ft::FrontType::CTP);
  ft::LoginParams params;

  if (front_index >= sizeof(kSimnowTradeAddr) / sizeof(kSimnowTradeAddr[0]))
    exit(-1);

  params.set_front_addr(kSimnowTradeAddr[front_index]);
  params.set_md_server_addr(kSimnowMdAddr[front_index]);
  params.set_broker_id(kBrokerID);
  params.set_investor_id(kInvestorID);
  params.set_passwd(kPasswd);
  params.set_auth_code(kAuthCode);
  params.set_app_id(kAppID);
  params.set_subscribed_list({"rb2009.SHFE"});

  if (!trader.login(params)) {
    exit(-1);
  }

  trader.sell_open("rb2009.SHFE", 1, ft::OrderType::FAK, 3200);
  // trader.buy_close("rb2009.SHFE", 41, ft::OrderType::FAK, 3500);

  // trader.buy_close("rb2009.SHFE", 41, ft::OrderType::FAK, 3500);

  MyStrategy strategy;
  trader.mount_strategy("rb2009.SHFE", &strategy);

  while (1) {
    sleep(1);
    trader.show_positions();
  }
}
