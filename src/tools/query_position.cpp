// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include <getopt.hpp>

#include "ipc/redis_position_helper.h"

int main() {
  std::string ticker = getarg("", "--ticker");
  uint64_t account = getarg(0ULL, "--account");

  if (account == 0) {
    fmt::print("Invalid account\n");
    exit(-1);
  }

  ft::RedisPositionGetter pos_helper;
  pos_helper.set_account(account);

  ft::Position pos{};
  pos_helper.get(ticker, &pos);
  fmt::print("{}\n", dump_position(pos));
}
