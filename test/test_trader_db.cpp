// Copyright [2020-present] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include <gtest/gtest.h>

#include "ft/component/trader_db.h"

using ft::Position;
using ft::TraderDB;

TEST(TraderDB, ReadWrite) {
  TraderDB trader_db;
  ASSERT_TRUE(trader_db.Init("127.0.0.1:6379", "", ""));

  Position pos{};
  pos.ticker_id = 10086;
  ASSERT_TRUE(trader_db.SetPosition("test_trader_db", "test_ticker", pos));

  Position pos_to_get;
  ASSERT_TRUE(trader_db.GetPosition("test_trader_db", "test_ticker", &pos_to_get));
  ASSERT_EQ(pos_to_get.ticker_id, 10086);
  ASSERT_EQ(memcmp(&pos_to_get, &pos, sizeof(Position)), 0);
  ASSERT_TRUE(trader_db.GetPosition("test_trader_db", "unknown_ticker_xxxx", &pos_to_get));
}
