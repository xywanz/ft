#include "gtest/gtest.h"
#include "ft/base/log.h"
#include "ft/component/networking.h"

#include <atomic>

using ft::NetworkNode;

static std::atomic<int> kOnConnectedCount{0};
static std::atomic<int> kOnRecvMsgCount{0};
static std::atomic<int> kOnDisconnetedCount{0};

class TestNode : public NetworkNode {
 public:
  void OnConnected(int conn_id) override { ++kOnConnectedCount; }
  void OnDisconnected(int conn_id) override { ++kOnDisconnetedCount; }
  void OnRecvMsg(int conn_id, const nlohmann::json& msg) override {
    ASSERT_EQ(msg["data"], "test");
    ++kOnRecvMsgCount;
  }
};

TEST(CommandService, Listen) {
  TestNode server;
  ASSERT_TRUE(server.StartServer(18838));

  TestNode client;
  int conn_id;
  ASSERT_TRUE(client.StartClient());
  ASSERT_TRUE(client.Connect(18838, &conn_id));

  std::this_thread::sleep_for(std::chrono::milliseconds(20));
  nlohmann::json msg;
  msg["data"] = "test";
  ASSERT_TRUE(client.SendMsg(1, msg));

  std::this_thread::sleep_for(std::chrono::milliseconds(20));
  client.Disconnect(conn_id);

  std::this_thread::sleep_for(std::chrono::milliseconds(20));
  ASSERT_EQ(kOnConnectedCount, 2);
  ASSERT_EQ(kOnRecvMsgCount, 1);
  ASSERT_EQ(kOnDisconnetedCount, 2);
}
