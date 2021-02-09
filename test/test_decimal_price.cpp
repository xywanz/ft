// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include <gtest/gtest.h>

#include "trading_server/order_book/limit_order.h"

using namespace ft::orderbook;

TEST(PriceFormat, Double2Decimal) {
  ASSERT_EQ(314UL, Double2DecimalPrice(0.0314));
  ASSERT_EQ(3140UL, Double2DecimalPrice(0.314));
  ASSERT_EQ(31400UL, Double2DecimalPrice(3.14));
  ASSERT_EQ(314000UL, Double2DecimalPrice(31.4));
  ASSERT_EQ(3140000UL, Double2DecimalPrice(314.0));
  ASSERT_EQ(31400000UL, Double2DecimalPrice(3140.0));
  ASSERT_EQ(314000000UL, Double2DecimalPrice(31400.0));
  ASSERT_EQ(3140000000UL, Double2DecimalPrice(314000.0));

  ASSERT_DOUBLE_EQ(0.0314, Deciaml2DoublePrice(314UL));
  ASSERT_DOUBLE_EQ(0.314, Deciaml2DoublePrice(3140UL));
  ASSERT_DOUBLE_EQ(3.14, Deciaml2DoublePrice(31400UL));
  ASSERT_DOUBLE_EQ(31.4, Deciaml2DoublePrice(314000UL));
  ASSERT_DOUBLE_EQ(314.0, Deciaml2DoublePrice(3140000UL));
  ASSERT_DOUBLE_EQ(3140.0, Deciaml2DoublePrice(31400000UL));
  ASSERT_DOUBLE_EQ(31400.0, Deciaml2DoublePrice(314000000UL));
  ASSERT_DOUBLE_EQ(314000.0, Deciaml2DoublePrice(3140000000UL));
}
