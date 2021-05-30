// Copyright [2020-present] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include <gtest/gtest.h>

#include <vector>

#include "ft/base/contract_table.h"
#include "ft/component/position/calculator.h"

using ft::Contract;
using ft::ContractTable;
using ft::Direction;
using ft::Offset;
using ft::Position;
using ft::PositionCalculator;

bool is_contractable_inited = [] {
  std::vector<Contract> contracts;
  contracts.resize(2);
  for (auto& contract : contracts) {
    contract.size = 1;
  }
  return ContractTable::Init(std::move(contracts));
}();

TEST(PositionCalculator, SetPosition) {
  ASSERT_TRUE(is_contractable_inited);

  PositionCalculator pos_calc{};

  Position pos_1{};
  pos_1.ticker_id = 1;
  pos_1.long_pos.holdings = 100;
  ASSERT_TRUE(pos_calc.SetPosition(pos_1));

  Position pos_2{};
  pos_2.ticker_id = 2;
  pos_2.short_pos.open_pending = 8;
  pos_calc.SetPosition(pos_2);

  auto* pos_1_got = pos_calc.GetPosition(1);
  ASSERT_EQ(memcmp(pos_1_got, &pos_1, sizeof(Position)), 0);
  auto* pos_2_got = pos_calc.GetPosition(2);
  ASSERT_EQ(memcmp(pos_2_got, &pos_2, sizeof(Position)), 0);

  ASSERT_EQ(pos_calc.GetPosition(0), nullptr);
  ASSERT_EQ(pos_calc.GetPosition(3), nullptr);
}

TEST(PositionCalculator, Update) {
  ASSERT_TRUE(is_contractable_inited);

  PositionCalculator pos_calc{};

  ASSERT_TRUE(pos_calc.UpdatePending(1, Direction::kBuy, Offset::kOpen, 8));
  auto* pos = pos_calc.GetPosition(1);
  ASSERT_TRUE(pos);
  ASSERT_EQ(pos->long_pos.open_pending, 8);
  ASSERT_EQ(pos->long_pos.close_pending, 0);
  ASSERT_EQ(pos->long_pos.holdings, 0);
  ASSERT_EQ(pos->short_pos.open_pending, 0);
  ASSERT_EQ(pos->short_pos.holdings, 0);
  ASSERT_EQ(pos->short_pos.close_pending, 0);

  ASSERT_TRUE(pos_calc.UpdateTraded(1, Direction::kBuy, Offset::kOpen, 4, 100));
  pos = pos_calc.GetPosition(1);
  ASSERT_TRUE(pos);
  ASSERT_EQ(pos->long_pos.open_pending, 4);
  ASSERT_EQ(pos->long_pos.close_pending, 0);
  ASSERT_EQ(pos->long_pos.holdings, 4);
  ASSERT_EQ(pos->short_pos.open_pending, 0);
  ASSERT_EQ(pos->short_pos.holdings, 0);
  ASSERT_EQ(pos->short_pos.close_pending, 0);
  ASSERT_DOUBLE_EQ(pos->long_pos.cost_price, 100);

  ASSERT_TRUE(pos_calc.UpdateTraded(1, Direction::kBuy, Offset::kOpen, 4, 200));
  pos = pos_calc.GetPosition(1);
  ASSERT_TRUE(pos);
  ASSERT_EQ(pos->long_pos.open_pending, 0);
  ASSERT_EQ(pos->long_pos.close_pending, 0);
  ASSERT_EQ(pos->long_pos.holdings, 8);
  ASSERT_EQ(pos->short_pos.open_pending, 0);
  ASSERT_EQ(pos->short_pos.holdings, 0);
  ASSERT_EQ(pos->short_pos.close_pending, 0);
  ASSERT_DOUBLE_EQ(pos->long_pos.cost_price, 150);
}

TEST(PositionCalculator, Callback) {
  ASSERT_TRUE(is_contractable_inited);

  PositionCalculator pos_calc{};
  pos_calc.SetCallback([](const Position& pos) {
    static int count = 0;
    switch (count) {
      case 0: {
        ASSERT_EQ(pos.long_pos.open_pending, 8);
        ASSERT_EQ(pos.long_pos.close_pending, 0);
        ASSERT_EQ(pos.long_pos.holdings, 0);
        ASSERT_EQ(pos.short_pos.open_pending, 0);
        ASSERT_EQ(pos.short_pos.holdings, 0);
        ASSERT_EQ(pos.short_pos.close_pending, 0);
        break;
      }
      case 1: {
        ASSERT_EQ(pos.long_pos.open_pending, 4);
        ASSERT_EQ(pos.long_pos.close_pending, 0);
        ASSERT_EQ(pos.long_pos.holdings, 4);
        ASSERT_EQ(pos.short_pos.open_pending, 0);
        ASSERT_EQ(pos.short_pos.holdings, 0);
        ASSERT_EQ(pos.short_pos.close_pending, 0);
        ASSERT_DOUBLE_EQ(pos.long_pos.cost_price, 100);
        break;
      }
      case 2: {
        ASSERT_EQ(pos.long_pos.open_pending, 0);
        ASSERT_EQ(pos.long_pos.close_pending, 0);
        ASSERT_EQ(pos.long_pos.holdings, 8);
        ASSERT_EQ(pos.short_pos.open_pending, 0);
        ASSERT_EQ(pos.short_pos.holdings, 0);
        ASSERT_EQ(pos.short_pos.close_pending, 0);
        ASSERT_DOUBLE_EQ(pos.long_pos.cost_price, 150);
        break;
      }
      default: {
        ASSERT_TRUE(false);
        break;
      }
    }
    ++count;
  });

  ASSERT_TRUE(pos_calc.UpdatePending(1, Direction::kBuy, Offset::kOpen, 8));
  ASSERT_TRUE(pos_calc.UpdateTraded(1, Direction::kBuy, Offset::kOpen, 4, 100));
  ASSERT_TRUE(pos_calc.UpdateTraded(1, Direction::kBuy, Offset::kOpen, 4, 200));
}
