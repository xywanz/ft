// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include <gtest/gtest.h>

#include "cep/rms/common/no_self_trade.h"

static uint64_t order_id = 1;

void GenLO(ft::Order* order, uint32_t direction, uint32_t offset,
           double price) {
  order->req.direction = direction;
  order->req.offset = offset;
  order->req.price = price;
  order->req.order_id = order_id++;
  order->req.type = ft::OrderType::LIMIT;
}

void GenMO(ft::Order* order, uint32_t direction, uint32_t offset) {
  order->req.direction = direction;
  order->req.offset = offset;
  order->req.price = 0;
  order->req.order_id = order_id++;
  order->req.type = ft::OrderType::MARKET;
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

  ft::NoSelfTradeRule rule;
  rule.init(&params);

  GenLO(&order, ft::Direction::BUY, ft::Offset::OPEN, 100.1);
  order_map.emplace(static_cast<uint64_t>(order.req.order_id), order);

  GenLO(&order, ft::Direction::BUY, ft::Offset::OPEN, 100.1);
  ASSERT_EQ(ft::NO_ERROR, rule.check_order_req(&order));

  GenLO(&order, ft::Direction::BUY, ft::Offset::OPEN, 99.0);
  ASSERT_EQ(ft::NO_ERROR, rule.check_order_req(&order));

  GenLO(&order, ft::Direction::BUY, ft::Offset::OPEN, 110.0);
  ASSERT_EQ(ft::NO_ERROR, rule.check_order_req(&order));

  GenLO(&order, ft::Direction::BUY, ft::Offset::CLOSE, 100.1);
  ASSERT_EQ(ft::NO_ERROR, rule.check_order_req(&order));

  GenLO(&order, ft::Direction::BUY, ft::Offset::CLOSE, 99.0);
  ASSERT_EQ(ft::NO_ERROR, rule.check_order_req(&order));

  GenLO(&order, ft::Direction::BUY, ft::Offset::CLOSE, 110.0);
  ASSERT_EQ(ft::NO_ERROR, rule.check_order_req(&order));

  GenLO(&order, ft::Direction::BUY, ft::Offset::CLOSE_TODAY, 100.1);
  ASSERT_EQ(ft::NO_ERROR, rule.check_order_req(&order));

  GenLO(&order, ft::Direction::BUY, ft::Offset::CLOSE_TODAY, 99.0);
  ASSERT_EQ(ft::NO_ERROR, rule.check_order_req(&order));

  GenLO(&order, ft::Direction::BUY, ft::Offset::CLOSE_TODAY, 110.0);
  ASSERT_EQ(ft::NO_ERROR, rule.check_order_req(&order));

  GenLO(&order, ft::Direction::BUY, ft::Offset::CLOSE_YESTERDAY, 100.1);
  ASSERT_EQ(ft::NO_ERROR, rule.check_order_req(&order));

  GenLO(&order, ft::Direction::BUY, ft::Offset::CLOSE_YESTERDAY, 99.0);
  ASSERT_EQ(ft::NO_ERROR, rule.check_order_req(&order));

  GenLO(&order, ft::Direction::BUY, ft::Offset::CLOSE_YESTERDAY, 110.0);
  ASSERT_EQ(ft::NO_ERROR, rule.check_order_req(&order));

  GenLO(&order, ft::Direction::SELL, ft::Offset::OPEN, 110.0);
  ASSERT_EQ(ft::NO_ERROR, rule.check_order_req(&order));

  GenLO(&order, ft::Direction::SELL, ft::Offset::OPEN, 100.1);
  ASSERT_EQ(ft::ERR_SELF_TRADE, rule.check_order_req(&order));

  GenLO(&order, ft::Direction::SELL, ft::Offset::OPEN, 99.0);
  ASSERT_EQ(ft::ERR_SELF_TRADE, rule.check_order_req(&order));

  GenLO(&order, ft::Direction::SELL, ft::Offset::CLOSE, 110.0);
  ASSERT_EQ(ft::NO_ERROR, rule.check_order_req(&order));

  GenLO(&order, ft::Direction::SELL, ft::Offset::CLOSE, 100.1);
  ASSERT_EQ(ft::ERR_SELF_TRADE, rule.check_order_req(&order));

  GenLO(&order, ft::Direction::SELL, ft::Offset::CLOSE, 99.0);
  ASSERT_EQ(ft::ERR_SELF_TRADE, rule.check_order_req(&order));

  GenLO(&order, ft::Direction::SELL, ft::Offset::CLOSE_TODAY, 110.0);
  ASSERT_EQ(ft::NO_ERROR, rule.check_order_req(&order));

  GenLO(&order, ft::Direction::SELL, ft::Offset::CLOSE_TODAY, 100.1);
  ASSERT_EQ(ft::ERR_SELF_TRADE, rule.check_order_req(&order));

  GenLO(&order, ft::Direction::SELL, ft::Offset::CLOSE_TODAY, 99.0);
  ASSERT_EQ(ft::ERR_SELF_TRADE, rule.check_order_req(&order));

  GenLO(&order, ft::Direction::SELL, ft::Offset::CLOSE_YESTERDAY, 110.0);
  ASSERT_EQ(ft::NO_ERROR, rule.check_order_req(&order));

  GenLO(&order, ft::Direction::SELL, ft::Offset::CLOSE_YESTERDAY, 100.1);
  ASSERT_EQ(ft::ERR_SELF_TRADE, rule.check_order_req(&order));

  GenLO(&order, ft::Direction::SELL, ft::Offset::CLOSE_YESTERDAY, 99.0);
  ASSERT_EQ(ft::ERR_SELF_TRADE, rule.check_order_req(&order));

  GenMO(&order, ft::Direction::BUY, ft::Offset::OPEN);
  ASSERT_EQ(ft::NO_ERROR, rule.check_order_req(&order));

  GenMO(&order, ft::Direction::BUY, ft::Offset::CLOSE);
  ASSERT_EQ(ft::NO_ERROR, rule.check_order_req(&order));

  GenMO(&order, ft::Direction::BUY, ft::Offset::CLOSE_TODAY);
  ASSERT_EQ(ft::NO_ERROR, rule.check_order_req(&order));

  GenMO(&order, ft::Direction::BUY, ft::Offset::CLOSE_YESTERDAY);
  ASSERT_EQ(ft::NO_ERROR, rule.check_order_req(&order));

  GenMO(&order, ft::Direction::SELL, ft::Offset::OPEN);
  ASSERT_EQ(ft::ERR_SELF_TRADE, rule.check_order_req(&order));

  GenMO(&order, ft::Direction::SELL, ft::Offset::CLOSE);
  ASSERT_EQ(ft::ERR_SELF_TRADE, rule.check_order_req(&order));

  GenMO(&order, ft::Direction::SELL, ft::Offset::CLOSE_TODAY);
  ASSERT_EQ(ft::ERR_SELF_TRADE, rule.check_order_req(&order));

  GenMO(&order, ft::Direction::SELL, ft::Offset::CLOSE_YESTERDAY);
  ASSERT_EQ(ft::ERR_SELF_TRADE, rule.check_order_req(&order));
}
