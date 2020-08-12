// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include <gtest/gtest.h>
#include <spdlog/spdlog.h>

int main(int argc, char** argv) {
  spdlog::set_level(spdlog::level::off);
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
