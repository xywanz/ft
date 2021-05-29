// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include <gtest/gtest.h>

#include "trader/risk/common/self_trade_risk.h"

static uint64_t order_id = 1;

void GenLO(ft::Order* order, ft::Direction direction, ft::Offset offset, double price) {
  order->req.direction = direction;
  order->req.offset = offset;
  order->req.price = price;
  order->req.order_id = order_id++;
  order->req.type = ft::OrderType::kLimit;
}

void GenMO(ft::Order* order, ft::Direction direction, ft::Offset offset) {
  order->req.direction = direction;
  order->req.offset = offset;
  order->req.price = 0;
  order->req.order_id = order_id++;
  order->req.type = ft::OrderType::kMarket;
}

TEST(RMS, NoSelfTrade) {
  ft::RiskRuleParams params{};
  ft::OrderMap order_map;
  ft::Contract contract{};
  ft::Order order{};

  params.order_map = &order_map;
  contract.ticker = "ticker001";
  order.req.contract = &contract;
  order.req.volume = 1;

  ft::SelfTradeRisk rule;
  rule.Init(&params);

  GenLO(&order, ft::Direction::kBuy, ft::Offset::kOpen, 100.1);
  order_map.emplace(static_cast<uint64_t>(order.req.order_id), order);

  GenLO(&order, ft::Direction::kBuy, ft::Offset::kOpen, 100.1);
  ASSERT_EQ(ft::ErrorCode::kNoError, rule.CheckOrderRequest(order));

  GenLO(&order, ft::Direction::kBuy, ft::Offset::kOpen, 99.0);
  ASSERT_EQ(ft::ErrorCode::kNoError, rule.CheckOrderRequest(order));

  GenLO(&order, ft::Direction::kBuy, ft::Offset::kOpen, 110.0);
  ASSERT_EQ(ft::ErrorCode::kNoError, rule.CheckOrderRequest(order));

  GenLO(&order, ft::Direction::kBuy, ft::Offset::kClose, 100.1);
  ASSERT_EQ(ft::ErrorCode::kNoError, rule.CheckOrderRequest(order));

  GenLO(&order, ft::Direction::kBuy, ft::Offset::kClose, 99.0);
  ASSERT_EQ(ft::ErrorCode::kNoError, rule.CheckOrderRequest(order));

  GenLO(&order, ft::Direction::kBuy, ft::Offset::kClose, 110.0);
  ASSERT_EQ(ft::ErrorCode::kNoError, rule.CheckOrderRequest(order));

  GenLO(&order, ft::Direction::kBuy, ft::Offset::kCloseToday, 100.1);
  ASSERT_EQ(ft::ErrorCode::kNoError, rule.CheckOrderRequest(order));

  GenLO(&order, ft::Direction::kBuy, ft::Offset::kCloseToday, 99.0);
  ASSERT_EQ(ft::ErrorCode::kNoError, rule.CheckOrderRequest(order));

  GenLO(&order, ft::Direction::kBuy, ft::Offset::kCloseToday, 110.0);
  ASSERT_EQ(ft::ErrorCode::kNoError, rule.CheckOrderRequest(order));

  GenLO(&order, ft::Direction::kBuy, ft::Offset::kCloseYesterday, 100.1);
  ASSERT_EQ(ft::ErrorCode::kNoError, rule.CheckOrderRequest(order));

  GenLO(&order, ft::Direction::kBuy, ft::Offset::kCloseYesterday, 99.0);
  ASSERT_EQ(ft::ErrorCode::kNoError, rule.CheckOrderRequest(order));

  GenLO(&order, ft::Direction::kBuy, ft::Offset::kCloseYesterday, 110.0);
  ASSERT_EQ(ft::ErrorCode::kNoError, rule.CheckOrderRequest(order));

  GenLO(&order, ft::Direction::kSell, ft::Offset::kOpen, 110.0);
  ASSERT_EQ(ft::ErrorCode::kNoError, rule.CheckOrderRequest(order));

  GenLO(&order, ft::Direction::kSell, ft::Offset::kOpen, 100.1);
  ASSERT_EQ(ft::ErrorCode::kSelfTrade, rule.CheckOrderRequest(order));

  GenLO(&order, ft::Direction::kSell, ft::Offset::kOpen, 99.0);
  ASSERT_EQ(ft::ErrorCode::kSelfTrade, rule.CheckOrderRequest(order));

  GenLO(&order, ft::Direction::kSell, ft::Offset::kClose, 110.0);
  ASSERT_EQ(ft::ErrorCode::kNoError, rule.CheckOrderRequest(order));

  GenLO(&order, ft::Direction::kSell, ft::Offset::kClose, 100.1);
  ASSERT_EQ(ft::ErrorCode::kSelfTrade, rule.CheckOrderRequest(order));

  GenLO(&order, ft::Direction::kSell, ft::Offset::kClose, 99.0);
  ASSERT_EQ(ft::ErrorCode::kSelfTrade, rule.CheckOrderRequest(order));

  GenLO(&order, ft::Direction::kSell, ft::Offset::kCloseToday, 110.0);
  ASSERT_EQ(ft::ErrorCode::kNoError, rule.CheckOrderRequest(order));

  GenLO(&order, ft::Direction::kSell, ft::Offset::kCloseToday, 100.1);
  ASSERT_EQ(ft::ErrorCode::kSelfTrade, rule.CheckOrderRequest(order));

  GenLO(&order, ft::Direction::kSell, ft::Offset::kCloseToday, 99.0);
  ASSERT_EQ(ft::ErrorCode::kSelfTrade, rule.CheckOrderRequest(order));

  GenLO(&order, ft::Direction::kSell, ft::Offset::kCloseYesterday, 110.0);
  ASSERT_EQ(ft::ErrorCode::kNoError, rule.CheckOrderRequest(order));

  GenLO(&order, ft::Direction::kSell, ft::Offset::kCloseYesterday, 100.1);
  ASSERT_EQ(ft::ErrorCode::kSelfTrade, rule.CheckOrderRequest(order));

  GenLO(&order, ft::Direction::kSell, ft::Offset::kCloseYesterday, 99.0);
  ASSERT_EQ(ft::ErrorCode::kSelfTrade, rule.CheckOrderRequest(order));

  GenMO(&order, ft::Direction::kBuy, ft::Offset::kOpen);
  ASSERT_EQ(ft::ErrorCode::kNoError, rule.CheckOrderRequest(order));

  GenMO(&order, ft::Direction::kBuy, ft::Offset::kClose);
  ASSERT_EQ(ft::ErrorCode::kNoError, rule.CheckOrderRequest(order));

  GenMO(&order, ft::Direction::kBuy, ft::Offset::kCloseToday);
  ASSERT_EQ(ft::ErrorCode::kNoError, rule.CheckOrderRequest(order));

  GenMO(&order, ft::Direction::kBuy, ft::Offset::kCloseYesterday);
  ASSERT_EQ(ft::ErrorCode::kNoError, rule.CheckOrderRequest(order));

  GenMO(&order, ft::Direction::kSell, ft::Offset::kOpen);
  ASSERT_EQ(ft::ErrorCode::kSelfTrade, rule.CheckOrderRequest(order));

  GenMO(&order, ft::Direction::kSell, ft::Offset::kClose);
  ASSERT_EQ(ft::ErrorCode::kSelfTrade, rule.CheckOrderRequest(order));

  GenMO(&order, ft::Direction::kSell, ft::Offset::kCloseToday);
  ASSERT_EQ(ft::ErrorCode::kSelfTrade, rule.CheckOrderRequest(order));

  GenMO(&order, ft::Direction::kSell, ft::Offset::kCloseYesterday);
  ASSERT_EQ(ft::ErrorCode::kSelfTrade, rule.CheckOrderRequest(order));
}
