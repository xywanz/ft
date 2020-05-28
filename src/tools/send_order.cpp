// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include <getopt.hpp>

#include "common/order_sender.h"

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
  } else {
    printf("unknown direction: %s\n", direction.c_str());
    exit(-1);
  }

  uint64_t o;
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

  uint64_t k;
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

  ft::OrderSender sender;
  sender.set_account(account);
  sender.send_order(ticker, volume, d, o, k, price, 0);
}
