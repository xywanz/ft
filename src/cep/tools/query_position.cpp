// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include <getopt.hpp>

#include "ipc/redis_position_helper.h"

int main() {
  std::string ticker = getarg("", "--ticker");
  uint64_t account = getarg(0ULL, "--account");

  if (account == 0) {
    printf("Invalid account\n");
    exit(-1);
  }

  ft::RedisPositionGetter pos_helper;
  pos_helper.set_account(account);

  ft::Position pos{};
  pos_helper.get(ticker, &pos);
  auto& lp = pos.long_pos;
  auto& sp = pos.short_pos;
  printf("Position: %s\n", ticker.c_str());
  printf(
      "  Long:  { holdings:%d, yd_holdings:%d, frozen:%d, open_pending:%d, "
      "close_pending:%d, cost_price:%.3lf }\n",
      lp.holdings, lp.yd_holdings, lp.frozen, lp.open_pending, lp.close_pending,
      lp.cost_price);
  printf(
      "  Short: { holdings:%d, yd_holdings:%d, frozen:%d, open_pending:%d, "
      "close_pending:%d, cost_price:%.3lf }\n",
      sp.holdings, sp.yd_holdings, sp.frozen, sp.open_pending, sp.close_pending,
      sp.cost_price);
}
