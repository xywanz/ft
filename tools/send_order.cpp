// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include <ctime>
#include <thread>

#include "ft/strategy/order_sender.h"
#include "ft/utils/getopt.hpp"

static void Usage() {
  printf("Usage:\n");
  printf("    --mq                报单队列名称\n");
  printf("    --contracts         合约列表文件\n");
  printf("    --direction         buy, sell, purchase or redeem\n");
  printf("    --offset            open, Close, close_today or close_yesterday\n");
  printf("    --order_type        limit, market, fak or fok\n");
  printf("    -h, -?, --help      帮助\n");
  printf("    --price             价格\n");
  printf("    --ticker            ticker\n");
  printf("    --volume            数量\n");
}

int main() {
  std::string contracts_file = getarg("../config/contracts.csv", "--contracts");
  std::string ticker = getarg("", "--ticker");
  std::string direction = getarg("", "--direction");
  std::string offset = getarg("open", "--offset");
  std::string order_type = getarg("fak", "--order_type");
  std::string trade_mq_name = getarg("", "--mq");
  int volume = getarg(0, "--volume");
  double price = getarg(0.0, "--price");
  bool help = getarg(false, "-h", "--help", "-?");

  if (help) {
    Usage();
    exit(0);
  }

  if (!ft::ContractTable::Init(contracts_file)) {
    printf("ContractTable Init failed\n");
    exit(-1);
  }

  auto contract = ft::ContractTable::get_by_ticker(ticker);
  if (!contract) {
    printf("Unknown ticker: %s\n", ticker.c_str());
    exit(-1);
  }

  if (volume <= 0) {
    printf("Invalid volume: %d\n", volume);
    exit(-1);
  }

  ft::Direction d;
  if (direction == "buy") {
    d = ft::Direction::kBuy;
  } else if (direction == "sell") {
    d = ft::Direction::kSell;
  } else {
    printf("unknown direction: %s\n", direction.c_str());
    exit(-1);
  }

  ft::Offset o = ft::Offset::kUnknown;
  ft::OrderType k = ft::OrderType::kUnknown;
  if (d == ft::Direction::kBuy || d == ft::Direction::kSell) {
    if (offset == "open") {
      o = ft::Offset::kOpen;
    } else if (offset == "close") {
      o = ft::Offset::kClose;
    } else if (offset == "close_today") {
      o = ft::Offset::kCloseToday;
    } else if (offset == "close_yesterday") {
      o = ft::Offset::kCloseYesterday;
    } else {
      printf("unknown offset: %s\n", offset.c_str());
      exit(-1);
    }

    if (order_type == "limit") {
      k = ft::OrderType::kLimit;
    } else if (order_type == "market") {
      k = ft::OrderType::kMarket;
    } else if (order_type == "fak") {
      k = ft::OrderType::kFak;
    } else if (order_type == "fok") {
      k = ft::OrderType::kFok;
    } else {
      printf("unknown order type: %s\n", order_type.c_str());
      exit(-1);
    }
  }

  ft::OrderSender sender;
  sender.Init(trade_mq_name);

  timespec ts;
  clock_gettime(CLOCK_REALTIME, &ts);

  sender.SendOrder(ticker, volume, d, o, k, price, 0, ts.tv_sec * 1000000UL + ts.tv_nsec / 1000UL);
}
