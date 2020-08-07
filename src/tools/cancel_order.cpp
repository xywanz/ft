// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include <getopt.hpp>

#include "strategy/order_sender.h"

int main() {
  std::string ticker = getarg("", "--ticker");
  uint64_t account = getarg(0ULL, "--account");
  uint64_t order_id = getarg(0ULL, "--order_id");

  if (account == 0) {
    printf("Invalid account\n");
    exit(-1);
  }

  ft::OrderSender sender;
  sender.set_account(account);

  if (order_id != 0)
    sender.cancel_order(order_id);
  else if (ticker == "all")
    sender.cancel_all();
  else
    sender.cancel_for_ticker(ticker);
}
