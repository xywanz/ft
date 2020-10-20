// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include <getopt.hpp>

#include "strategy/order_sender.h"

static void usage() {
  printf("usage:\n");
  printf("    --account           账户\n");
  printf("    --contracts-file    合约列表文件\n");
  printf("    --direction         buy, sell, purchase or redeem\n");
  printf(
      "    --offset            open, close, close_today or close_yesterday\n");
  printf("    --order_type        limit, market, fak or fok\n");
  printf("    -h, -?, --help      帮助\n");
  printf("    --price             价格\n");
  printf("    --ticker            ticker\n");
  printf("    --volume            数量\n");
}

int main() {
  std::string contracts_file =
      getarg("../config/contracts.csv", "--contracts-file");
  std::string ticker = getarg("", "--ticker");
  std::string direction = getarg("", "--direction");
  std::string offset = getarg("open", "--offset");
  std::string order_type = getarg("fak", "--order_type");
  uint64_t account = getarg(0ULL, "--account");
  int volume = getarg(0, "--volume");
  double price = getarg(0.0, "--price");
  bool help = getarg(false, "-h", "--help", "-?");

  if (help) {
    usage();
    exit(0);
  }

  if (account == 0) {
    printf("Invalid account\n");
    exit(-1);
  }

  if (!ft::ContractTable::init(contracts_file)) {
    printf("ContractTable init failed\n");
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

  uint64_t d;
  if (direction == "buy") {
    d = ft::Direction::BUY;
  } else if (direction == "sell") {
    d = ft::Direction::SELL;
  } else if (direction == "purchase") {
    d = ft::Direction::PURCHASE;
  } else if (direction == "redeem") {
    d = ft::Direction::REDEEM;
  } else {
    printf("unknown direction: %s\n", direction.c_str());
    exit(-1);
  }

  uint64_t o = 0;
  uint64_t k = 0;
  if (d == ft::Direction::BUY || d == ft::Direction::SELL) {
    if (offset == "open") {
      o = ft::Offset::OPEN;
    } else if (offset == "close") {
      o = ft::Offset::CLOSE;
    } else if (offset == "close_today") {
      o = ft::Offset::CLOSE_TODAY;
    } else if (offset == "close_yesterday") {
      o = ft::Offset::CLOSE_YESTERDAY;
    } else {
      printf("unknown offset: %s\n", offset.c_str());
      exit(-1);
    }

    if (order_type == "limit") {
      k = ft::OrderType::LIMIT;
    } else if (order_type == "market") {
      k = ft::OrderType::MARKET;
    } else if (order_type == "fak") {
      k = ft::OrderType::FAK;
    } else if (order_type == "fok") {
      k = ft::OrderType::FOK;
    } else {
      printf("unknown order type: %s\n", order_type.c_str());
      exit(-1);
    }
  }

  ft::OrderSender sender;
  sender.set_account(account);
  sender.send_order(ticker, volume, d, o, k, price, 0);
}
