// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include <gtest/gtest.h>

#include <chrono>
#include <string>

#include "ft/component/serializable.h"

struct MyData : public ft::pubsub::Serializable<MyData> {
  int a;
  int b;
  float c;
  double d[10];

  struct {
    int a;
  } s;

  bool operator==(const MyData& rhs) const {
    bool equal = (a == rhs.a && b == rhs.b && c == rhs.c && s.a == rhs.s.a);
    for (int i = 0; i < 10; ++i) {
      equal = (equal && d[i] == rhs.d[i]);
    }
    return equal;
  }

  SERIALIZABLE_FIELDS(a, b, c, d, s.a);
};

TEST(cereal, test_0) {
  MyData my_data{};
  my_data.a = 1;
  my_data.b = 2;
  my_data.c = 3.0f;
  my_data.s.a = 88;
  for (int i = 0; i < 10; ++i) {
    my_data.d[i] = i;
  }

  std::string binary;
  my_data.SerializeToString(&binary);

  MyData my_data_2{};
  my_data_2.ParseFromString(binary);

  ASSERT_EQ(my_data, my_data_2);
}
