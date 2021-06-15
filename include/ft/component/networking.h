// Copyright [2020-present] <Copyright Kevin, kevin.lau.gd@gmail.com>

#pragma once

#include <atomic>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>

#include "nlohmann/json.hpp"
#include "uv.h"

namespace ft {

class NetworkNode {
 private:
  enum TaskType {
    kConnect,
    kDisconnect,
    kSend,
  };

  struct Task {
    int conn_id;
    int type;
    uint64_t args[4];
  };

 public:
  struct Connection {
    NetworkNode* node;
    uv_tcp_t client;
    int conn_id;
    std::vector<char> rcv_buf;
    std::vector<char> snd_buf;
  };

 public:
  NetworkNode();
  ~NetworkNode();

  int GenConnId();
  bool Connect(int port, int conn_id);
  bool Connect(int port);
  bool Disconnect(int conn_id);

  bool StartServer(int port);
  bool StartClient();

  bool SendMsg(int conn_id, const nlohmann::json& msg);

  void OnUvConnect(int conn_id, int status);
  void OnUvConnection(int status);
  void OnUvRead(int conn_id, const char* data, ssize_t len);
  void OnUvAsyncTask();
  void OnUvAsyncExit();

 protected:
  virtual void OnConnected(int conn_id) {}
  virtual void OnDisconnected(int conn_id) {}
  virtual void OnRecvMsg(int conn_id, const nlohmann::json& msg) {}

 private:
  void DoConnect(int conn_id, int port);
  void DoDisconnect(int conn_id);
  void DoSendMsg(int conn_id, char* data, std::size_t size);

  void OnHeartBeat(int conn_id);

  void MainLoop();

  int next_conn_id() { return next_conn_id_++; }

  void PutTask(const Task& task);
  bool GetTask(Task* task);
  void Notify();

 private:
  uv_loop_t loop_;
  uv_tcp_t tcp_server_;
  std::thread main_loop_thread_;
  std::map<int, std::shared_ptr<Connection>> connections_;

  std::mutex mtx_;
  std::queue<Task> task_queue_;
  uv_async_t async_task_req_;

  std::atomic<bool> running_{false};
  std::atomic<int> next_conn_id_{1};
};

}  // namespace ft
