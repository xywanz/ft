// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "ft/utils/getopt.hpp"
#include "ft/utils/protocol_utils.h"
#include "ft/utils/redis_position_helper.h"

static void Usage() {
  printf("Usage:\n");
  printf("    --account           账户\n");
  printf("    -h, -?, --help      帮助\n");
  printf("    --ticker            ticker\n");
}

int main() {
  std::string ticker = getarg("", "--ticker");
  uint64_t account = getarg(0ULL, "--account");
  bool help = getarg(false, "-h", "--help", "-?");

  if (help) {
    Usage();
    exit(0);
  }

  if (account == 0) {
    fmt::print("Invalid account\n");
    exit(-1);
  }

  ft::RedisPositionGetter pos_helper;
  pos_helper.SetAccount(account);

  ft::Position pos{};
  pos_helper.get(ticker, &pos);
  fmt::print("{}\n", DumpPosition(pos));
}
