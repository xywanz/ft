// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include <gtest/gtest.h>

#include <thread>

#include "ft/utils/ring_buffer.h"

using ft::RingBuffer;

TEST(RingBufferTest, Case_0) {
  const int n = 10000000;
  RingBuffer<int, 4096> rb;

  std::thread rd_thread([&]() {
    for (int i = 0; i < n; ++i) {
      int res;
      rb.GetWithBlocking(&res);
      ASSERT_EQ(res, i);
    }
  });

  std::thread wr_thread([&]() {
    for (int i = 0; i < n; ++i) {
      rb.PutWithBlocking(i);
    }
  });

  rd_thread.join();
  wr_thread.join();
}
