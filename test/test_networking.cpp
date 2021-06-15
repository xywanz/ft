// Copyright [2020-present] <Copyright Kevin, kevin.lau.gd@gmail.com>
#include <atomic>

#include "ft/base/log.h"
#include "ft/component/networking.h"
#include "gtest/gtest.h"

using ft::NetworkNode;

static std::atomic<int> kOnConnectedCount{0};
static std::atomic<int> kOnRecvMsgCount{0};
static std::atomic<int> kOnDisconnetedCount{0};
static const char* expected_data = nullptr;

class TestNode : public NetworkNode {
 public:
  void OnConnected(int conn_id) override { ++kOnConnectedCount; }
  void OnDisconnected(int conn_id) override { ++kOnDisconnetedCount; }
  void OnRecvMsg(int conn_id, const nlohmann::json& msg) override {
    ASSERT_EQ(msg["data"], expected_data);
    ++kOnRecvMsgCount;
  }
};

TEST(NetworkNode, ListenAndConnect) {
  kOnConnectedCount = 0;
  kOnRecvMsgCount = 0;
  kOnDisconnetedCount = 0;

  int port = 18838;

  TestNode server;
  ASSERT_TRUE(server.StartServer(port));

  TestNode client;
  ASSERT_TRUE(client.StartClient());
  ASSERT_TRUE(client.Connect(port));

  std::this_thread::sleep_for(std::chrono::milliseconds(20));
  ASSERT_EQ(kOnConnectedCount, 2);
}

TEST(NetworkNode, Disconnect) {
  kOnConnectedCount = 0;
  kOnRecvMsgCount = 0;
  kOnDisconnetedCount = 0;

  int port = 18839;

  TestNode server;
  ASSERT_TRUE(server.StartServer(port));

  TestNode client;
  int conn_id = client.GenConnId();
  ASSERT_TRUE(client.StartClient());
  ASSERT_TRUE(client.Connect(port, conn_id));

  std::this_thread::sleep_for(std::chrono::milliseconds(20));
  client.Disconnect(conn_id);

  std::this_thread::sleep_for(std::chrono::milliseconds(20));
  ASSERT_EQ(kOnConnectedCount, 2);
  ASSERT_EQ(kOnDisconnetedCount, 2);
}

TEST(NetworkNode, SendMsg) {
  kOnConnectedCount = 0;
  kOnRecvMsgCount = 0;
  kOnDisconnetedCount = 0;

  int port = 18840;

  TestNode server;
  ASSERT_TRUE(server.StartServer(port));

  TestNode client;
  int conn_id = client.GenConnId();
  ASSERT_TRUE(client.StartClient());
  ASSERT_TRUE(client.Connect(port, conn_id));

  std::this_thread::sleep_for(std::chrono::milliseconds(20));
  nlohmann::json msg;
  expected_data = "test";
  msg["data"] = expected_data;
  ASSERT_TRUE(client.SendMsg(1, msg));

  std::this_thread::sleep_for(std::chrono::milliseconds(20));
  client.Disconnect(conn_id);

  std::this_thread::sleep_for(std::chrono::milliseconds(20));
  ASSERT_EQ(kOnConnectedCount, 2);
  ASSERT_EQ(kOnRecvMsgCount, 1);
  ASSERT_EQ(kOnDisconnetedCount, 2);
}

TEST(NetworkNode, MultiClients) {
  kOnConnectedCount = 0;
  kOnRecvMsgCount = 0;
  kOnDisconnetedCount = 0;

  int port = 18841;

  TestNode server;
  ASSERT_TRUE(server.StartServer(port));

  TestNode client_1;
  int conn_id_1 = client_1.GenConnId();
  ASSERT_TRUE(client_1.StartClient());
  ASSERT_TRUE(client_1.Connect(port, conn_id_1));
  std::this_thread::sleep_for(std::chrono::milliseconds(20));
  nlohmann::json msg_1;
  expected_data = "test_1";
  msg_1["data"] = expected_data;
  ASSERT_TRUE(client_1.SendMsg(1, msg_1));
  std::this_thread::sleep_for(std::chrono::milliseconds(20));

  TestNode client_2;
  int conn_id_2 = client_2.GenConnId();
  ASSERT_TRUE(client_2.StartClient());
  ASSERT_TRUE(client_2.Connect(port, conn_id_2));
  std::this_thread::sleep_for(std::chrono::milliseconds(20));
  nlohmann::json msg_2;
  expected_data = "test_2";
  msg_2["data"] = expected_data;
  ASSERT_TRUE(client_2.SendMsg(1, msg_2));

  std::this_thread::sleep_for(std::chrono::milliseconds(20));
  client_1.Disconnect(conn_id_1);
  client_2.Disconnect(conn_id_2);

  std::this_thread::sleep_for(std::chrono::milliseconds(20));
  ASSERT_EQ(kOnConnectedCount, 4);
  ASSERT_EQ(kOnRecvMsgCount, 2);
  ASSERT_EQ(kOnDisconnetedCount, 4);
}
