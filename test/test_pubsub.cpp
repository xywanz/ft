// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include <ft/component/pubsub/publisher.h>
#include <ft/component/pubsub/subscriber.h>
#include <ft/utils/datetime.h>
#include <gtest/gtest.h>

#include <thread>

using ft::pubsub::Publisher;
using ft::pubsub::Subscriber;

TEST(PubSubTest, Case_0) {
  auto now = ft::datetime::Datetime::now();

  Publisher pub("ipc://test.ipc");

  Subscriber sub("ipc://test.ipc");
  std::function<void(ft::datetime::Datetime*)> cb = [&](ft::datetime::Datetime* data) {
    ASSERT_EQ(*data, now);
  };
  sub.Subscribe("A", cb);
  std::thread th([&]() {
    for (int i = 0; i < 100; ++i) {
      sub.GetReply();
    }
  });
  std::this_thread::sleep_for(std::chrono::milliseconds(20));

  for (int i = 0; i < 100; ++i) {
    ASSERT_TRUE(pub.Publish("A", now));
  }

  th.join();
}
