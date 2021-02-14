// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include <gtest/gtest.h>

#include <string>

#include "component/pubsub/serializable.h"

struct MyData : public ft::pubsub::Serializable<MyData> {
  int a;
  int b;
  float c;

  SERIALIZABLE_FIELDS(a, b, c);
};

TEST(cereal, test_0) {
  MyData my_data{};
  my_data.a = 1;
  my_data.b = 2;
  my_data.c = 3.0f;

  std::string binary;
  my_data.SerializeToString(&binary);

  MyData my_data_2{};
  my_data_2.ParseFromString(binary);

  ASSERT_EQ(my_data.a, my_data_2.a);
  ASSERT_EQ(my_data.b, my_data_2.b);
  ASSERT_EQ(my_data.c, my_data_2.c);
}
