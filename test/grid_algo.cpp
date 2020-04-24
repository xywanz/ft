// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include <iostream>

#include <getopt.hpp>
#include <spdlog/spdlog.h>

#include "Strategy.h"
#include "StrategyEngine.h"
#include "TradingSystem.h"

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
  void on_init(ft::QuantitativeTradingContext* ctx) override {
    spdlog::info("[MyStrategy::on_init]");

    const auto* pos = ctx->get_position();
    const auto& lp = pos->long_pos;
    const auto& sp = pos->short_pos;

    if (lp.volume > 0) {
      ctx->sell(lp.volume, 3300);
      spdlog::info("Close all long pos");
    }

    if (sp.volume > 0) {
      ctx->buy(sp.volume, 3600);
      spdlog::info("Close all short pos");
    }
  }

  void on_tick(ft::QuantitativeTradingContext* ctx) override {
    const auto* tick = ctx->get_tick();

    if (last_grid_price_ < 1e-6)
      last_grid_price_ = tick->last_price;

    const auto* pos = ctx->get_position();
    const auto& lp = pos->long_pos;
    const auto& sp = pos->short_pos;

    spdlog::info(
      "[MyStrategy::on_tick] last_price: {:.2f}, grid: {:.2f}, long: {}, short: {}, trades: {}",
      ctx->get_tick()->last_price, last_grid_price_, lp.volume, sp.volume, trade_counts_);

    if (tick->last_price - last_grid_price_ > grid_height_ - 1e-6) {
      ctx->sell(trade_volume_each_, tick->bid[0]);
      spdlog::info("[GRID] SELL VOLUME: {}, PRICE: {:.2f}, LAST:{:.2f}, PREV: {:.2f}",
                   trade_volume_each_, tick->bid[0], tick->last_price, last_grid_price_);
      last_grid_price_ = tick->last_price;
      ++trade_counts_;
    } else if (tick->last_price - last_grid_price_ < -grid_height_ + 1e-6) {
      ctx->buy(trade_volume_each_, tick->ask[0]);
      spdlog::info("[GRID] BUY VOLUME: {}, PRICE: {:.2f}, LAST:{:.2f}, PREV: {:.2f}",
                   trade_volume_each_, tick->ask[0], tick->last_price, last_grid_price_);
      last_grid_price_ = tick->last_price;
      ++trade_counts_;
    }
  }

  void on_exit(ft::QuantitativeTradingContext* ctx) override {
    spdlog::info("[MyStrategy::on_exit]");

    const auto* pos = ctx->get_position();
    const auto& lp = pos->long_pos;
    const auto& sp = pos->short_pos;

    if (lp.volume > 0) {
      ctx->sell(lp.volume, 3300);
      spdlog::info("Close all long pos");
    }

    if (sp.volume > 0) {
      ctx->buy(sp.volume, 3600);
      spdlog::info("Close all short pos");
    }
  }

 private:
  double last_grid_price_ = 0.0;
  double grid_height_ = 10.0;
  int trade_volume_each_ = 100;
  int trade_counts_ = 0;
};


int main() {
  int         log_level   = getarg(2,   "--loglevel");
  std::size_t front_index = getarg(0UL, "--front");

  spdlog::set_level(static_cast<spdlog::level::level_enum>(log_level));
  ft::ContractTable::init("./contracts.csv");
  ft::StrategyEngine se(ft::FrontType::CTP);
  const std::string ticker = "rb2009.SHFE";

  if (front_index >= sizeof(kSimnowTradeAddr) / sizeof(kSimnowTradeAddr[0]))
    exit(-1);

  ft::LoginParams params;
  params.set_front_addr(kSimnowTradeAddr[front_index]);
  params.set_md_server_addr(kSimnowMdAddr[front_index]);
  params.set_broker_id(kBrokerID);
  params.set_investor_id(kInvestorID);
  params.set_passwd(kPasswd);
  params.set_auth_code(kAuthCode);
  params.set_app_id(kAppID);
  params.set_subscribed_list({ticker});

  if (!se.login(params))
    exit(-1);

  MyStrategy* strategy = new MyStrategy;
  se.mount_strategy(ticker, strategy);

  while (1) {
    sleep(60);
  }
}
