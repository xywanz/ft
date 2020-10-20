// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include <getopt.hpp>

#include "ipc/redis_position_helper.h"

static void usage() {
  printf("usage:\n");
  printf("    --account           账户\n");
  printf("    -h, -?, --help      帮助\n");
  printf("    --ticker            ticker\n");
}

int main() {
  std::string ticker = getarg("", "--ticker");
  uint64_t account = getarg(0ULL, "--account");
  bool help = getarg(false, "-h", "--help", "-?");

  if (help) {
    usage();
    exit(0);
  }

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
