// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "ft/component/trader_db.h"
#include "ft/utils/getopt.hpp"
#include "ft/utils/protocol_utils.h"

static void Usage() {
  printf("Usage:\n");
  printf("    -h, -?, --help      帮助\n");
  printf("    --ticker            ticker\n");
  printf("    --strategy          strategy name\n");
}

int main() {
  std::string ticker = getarg("", "--ticker");
  std::string strategy = getarg("common", "--strategy");
  bool help = getarg(false, "-h", "--help", "-?");

  if (help) {
    Usage();
    exit(EXIT_SUCCESS);
  }

  ft::TraderDB trader_db;
  if (!trader_db.Init("127.0.0.1:6379", "", "")) {
    fmt::print("failed to open db connection");
    exit(EXIT_FAILURE);
  }

  ft::Position pos{};
  trader_db.GetPosition(strategy, ticker, &pos);
  fmt::print("{}\n", DumpPosition(pos));
}
