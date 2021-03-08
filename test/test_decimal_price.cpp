// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include <gtest/gtest.h>

#include "ft/component/order_book/limit_order.h"

using ft::orderbook::price_decimal_to_double;
using ft::orderbook::price_double_to_decimal;

TEST(PriceFormat, Double2Decimal) {
  ASSERT_EQ(314UL, price_double_to_decimal(0.0314));
  ASSERT_EQ(3140UL, price_double_to_decimal(0.314));
  ASSERT_EQ(31400UL, price_double_to_decimal(3.14));
  ASSERT_EQ(314000UL, price_double_to_decimal(31.4));
  ASSERT_EQ(3140000UL, price_double_to_decimal(314.0));
  ASSERT_EQ(31400000UL, price_double_to_decimal(3140.0));
  ASSERT_EQ(314000000UL, price_double_to_decimal(31400.0));
  ASSERT_EQ(3140000000UL, price_double_to_decimal(314000.0));

  ASSERT_DOUBLE_EQ(0.0314, price_decimal_to_double(314UL));
  ASSERT_DOUBLE_EQ(0.314, price_decimal_to_double(3140UL));
  ASSERT_DOUBLE_EQ(3.14, price_decimal_to_double(31400UL));
  ASSERT_DOUBLE_EQ(31.4, price_decimal_to_double(314000UL));
  ASSERT_DOUBLE_EQ(314.0, price_decimal_to_double(3140000UL));
  ASSERT_DOUBLE_EQ(3140.0, price_decimal_to_double(31400000UL));
  ASSERT_DOUBLE_EQ(31400.0, price_decimal_to_double(314000000UL));
  ASSERT_DOUBLE_EQ(314000.0, price_decimal_to_double(3140000000UL));
}
