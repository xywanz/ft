// Copyright [2020-present] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "ft/component/networking.h"

#include <stdexcept>

#include "ft/base/log.h"

namespace ft {

namespace {

void on_uv_connect(uv_connect_t* req, int status) {
  auto* client = reinterpret_cast<uv_tcp_t*>(req->handle);
  auto* conn = reinterpret_cast<NetworkNode::Connection*>(client->data);
  reinterpret_cast<NetworkNode*>(req->data)->OnUvConnect(conn->conn_id, status);
  delete req;
}

void uv_connection_cb(uv_stream_t* server, int status) {
  reinterpret_cast<NetworkNode*>(server->data)->OnUvConnection(status);
}

void uv_alloc_cb(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf) {
  buf->base = new char[suggested_size];
  buf->len = suggested_size;
}

void uv_async_task(uv_async_t* handle) {
  reinterpret_cast<NetworkNode*>(handle->data)->OnUvAsyncTask();
}

void uv_async_exit(uv_async_t* handle) {
  reinterpret_cast<NetworkNode*>(handle->data)->OnUvAsyncExit();
}

auto uv_read_cb(uv_stream_t* client, ssize_t nread, const uv_buf_t* buf) {
  auto* conn = reinterpret_cast<NetworkNode::Connection*>(client->data);
  if (buf->base) {
    reinterpret_cast<NetworkNode*>(conn->node)->OnUvRead(conn->conn_id, buf->base, nread);
    delete[] buf->base;
  }
}

}  // namespace

NetworkNode::NetworkNode() {
  signal(SIGPIPE, SIG_IGN);
  uv_loop_init(&loop_);
  uv_async_init(&loop_, &async_task_req_, uv_async_task);
  async_task_req_.data = this;
}

NetworkNode::~NetworkNode() {
  if (running_) {
    uv_async_t async_exit;
    uv_async_init(&loop_, &async_exit, uv_async_exit);
    async_exit.data = this;
    uv_async_send(&async_exit);
    if (main_loop_thread_.joinable()) {
      main_loop_thread_.join();
    }
  }
}

bool NetworkNode::StartServer(int port) {
  struct sockaddr_in addr;
  if (uv_ip4_addr("127.0.0.1", port, &addr) != 0) {
    return false;
  }

  if (uv_tcp_init(&loop_, &tcp_server_) != 0) {
    return false;
  }
  tcp_server_.data = this;
  if (uv_tcp_bind(&tcp_server_, reinterpret_cast<struct sockaddr*>(&addr), 0) != 0) {
    return false;
  }
  if (uv_listen(reinterpret_cast<uv_stream_t*>(&tcp_server_), 64, uv_connection_cb) != 0) {
    return false;
  }

  running_ = true;
  main_loop_thread_ = std::thread(std::mem_fn(&NetworkNode::MainLoop), this);
  return true;
}

bool NetworkNode::StartClient() {
  running_ = true;
  main_loop_thread_ = std::thread(std::mem_fn(&NetworkNode::MainLoop), this);
  return true;
}

int NetworkNode::GenConnId() { return next_conn_id(); }

bool NetworkNode::Connect(int port, int conn_id) {
  Task task{};
  task.conn_id = conn_id;
  task.type = kConnect;
  task.args[0] = port;
  PutTask(task);
  Notify();

  return true;
}

bool NetworkNode::Connect(int port) { return Connect(port, GenConnId()); }

bool NetworkNode::Disconnect(int conn_id) {
  Task task{};
  task.conn_id = conn_id;
  task.type = kDisconnect;
  PutTask(task);
  Notify();
  return true;
}

bool NetworkNode::SendMsg(int conn_id, const nlohmann::json& msg) {
  auto msg_str = msg.dump();
  if (msg_str.size() == 0) {
    return true;
  }

  std::size_t data_size = msg_str.size() + sizeof(uint64_t);
  char* data = new char[data_size];
  *reinterpret_cast<uint64_t*>(data) = msg_str.size();
  memcpy(data + sizeof(uint64_t), msg_str.c_str(), msg_str.size());

  Task task{};
  task.conn_id = conn_id;
  task.type = kSend;
  task.args[0] = reinterpret_cast<uint64_t>(data);
  task.args[1] = data_size;
  PutTask(task);
  Notify();

  return true;
}

void NetworkNode::DoConnect(int conn_id, int port) {
  if (connections_.find(conn_id) != connections_.end()) {
    return;
  }

  auto conn = std::make_shared<Connection>();
  conn->conn_id = conn_id;
  conn->node = this;
  struct sockaddr_in addr;
  auto& client = conn->client;

  auto* connect_req = new uv_connect_t;
  connect_req->data = this;

  if (uv_ip4_addr("127.0.0.1", static_cast<int>(port), &addr) != 0) {
    goto handle_error;
  }

  if (uv_tcp_init(&loop_, &client) != 0) {
    goto handle_error;
  }
  client.data = conn.get();

  if (uv_tcp_connect(connect_req, &client, reinterpret_cast<struct sockaddr*>(&addr),
                     on_uv_connect) != 0) {
    uv_close(reinterpret_cast<uv_handle_t*>(&client), nullptr);
    goto handle_error;
  }

  connections_[conn_id] = conn;
  return;

handle_error:
  delete connect_req;
  OnDisconnected(conn_id);
}

void NetworkNode::DoDisconnect(int conn_id) {
  auto it = connections_.find(conn_id);
  if (it == connections_.end()) {
    return;
  }

  auto& conn = it->second;
  auto& client = conn->client;
  uv_close(reinterpret_cast<uv_handle_t*>(&client), nullptr);
  connections_.erase(it);
  OnDisconnected(conn_id);
}

void NetworkNode::DoSendMsg(int conn_id, char* data, std::size_t size) {
  std::shared_ptr<Connection> conn;
  uv_buf_t buf;
  std::size_t nsend = 0;

  auto it = connections_.find(conn_id);
  if (it == connections_.end() || conn_id != it->second->conn_id) {
    goto free_data;
  }

  conn = it->second;
  for (;;) {
    buf = uv_buf_init(data + nsend, size - nsend);
    int r = uv_try_write(reinterpret_cast<uv_stream_t*>(&conn->client), &buf, 1);
    if (r == UV_EAGAIN) {
      continue;
    } else if (r > 0) {
      nsend += r;
      if (nsend == size) {
        break;
      }
    } else {
      uv_close(reinterpret_cast<uv_handle_t*>(&conn->client), nullptr);
      connections_.erase(it);
      OnDisconnected(conn_id);
      break;
    }
  }

free_data:
  delete[] data;
}

void NetworkNode::OnHeartBeat(int conn_id) {}

void NetworkNode::MainLoop() {
  uv_run(&loop_, UV_RUN_DEFAULT);
  uv_loop_close(&loop_);
}

void NetworkNode::OnUvConnect(int conn_id, int status) {
  auto it = connections_.find(conn_id);
  if (it == connections_.end() || conn_id != it->second->conn_id) {
    return;
  }
  auto& conn = it->second;

  if (status < 0) {
    uv_close(reinterpret_cast<uv_handle_t*>(&conn->client), nullptr);
    connections_.erase(it);
    OnDisconnected(conn_id);
    return;
  }

  auto& client = conn->client;
  if (uv_read_start(reinterpret_cast<uv_stream_t*>(&client), uv_alloc_cb, uv_read_cb) != 0) {
    uv_close(reinterpret_cast<uv_handle_t*>(&client), nullptr);
    connections_.erase(conn_id);
    OnDisconnected(conn_id);
    return;
  }

  OnConnected(conn->conn_id);
}

void NetworkNode::OnUvConnection(int status) {
  if (status < 0) {
    LOG_ERROR("on_connect failed");
    return;
  }

  int conn_id = next_conn_id();
  if (connections_.find(conn_id) != connections_.end()) {
    // bug
    abort();
  }
  auto conn = std::make_shared<Connection>();
  conn->conn_id = conn_id;
  conn->node = this;
  auto& client = conn->client;
  if (uv_tcp_init(&loop_, &client) != 0) {
    return;
  }
  connections_[conn_id] = conn;
  client.data = conn.get();

  if (uv_accept(reinterpret_cast<uv_stream_t*>(&tcp_server_),
                reinterpret_cast<uv_stream_t*>(&client)) != 0) {
    goto handle_error;
  }

  if (uv_read_start(reinterpret_cast<uv_stream_t*>(&client), uv_alloc_cb, uv_read_cb) != 0) {
    goto handle_error;
  }

  OnConnected(conn_id);
  return;

handle_error:
  uv_close(reinterpret_cast<uv_handle_t*>(&client), nullptr);
  connections_.erase(conn_id);
  OnDisconnected(conn_id);
}

void NetworkNode::OnUvRead(int conn_id, const char* data, ssize_t len) {
  auto it = connections_.find(conn_id);
  if (it == connections_.end() || it->second->conn_id != conn_id) {
    return;
  }
  auto& conn = it->second;

  if (len > 0) {
    auto& buf = conn->rcv_buf;
    buf.insert(buf.end(), data, data + len);
    std::size_t pos = 0;
    while (pos + sizeof(uint64_t) <= buf.size() &&
           pos + sizeof(uint64_t) + *reinterpret_cast<uint64_t*>(&buf[pos]) <= buf.size()) {
      uint64_t msg_size = *reinterpret_cast<uint64_t*>(&buf[pos]);
      auto msg = nlohmann::json::parse(&buf[pos + sizeof(uint64_t)],
                                       &buf[pos + sizeof(uint64_t) + msg_size]);
      OnRecvMsg(conn_id, msg);
      pos += sizeof(uint64_t) + msg_size;
    }
    if (pos > 0) {
      std::vector<char> new_buf(buf.begin() + pos, buf.end());
      buf = std::move(new_buf);
    }
  } else if (len < 0) {
    if (len == UV_EOF) {
      LOG_DEBUG("disconnected. conn_id:{}", conn_id);
    } else {
      LOG_ERROR("error. conn_id:{}", conn_id);
    }
    uv_close(reinterpret_cast<uv_handle_t*>(&conn->client), nullptr);
    connections_.erase(it);
    OnDisconnected(conn_id);
  }
}

void NetworkNode::OnUvAsyncTask() {
  Task task{};
  while (GetTask(&task)) {
    if (task.type == kConnect) {
      DoConnect(task.conn_id, task.args[0]);
    } else if (task.type == kDisconnect) {
      DoDisconnect(task.conn_id);
    } else if (task.type == kSend) {
      DoSendMsg(task.conn_id, reinterpret_cast<char*>(task.args[0]), task.args[1]);
    } else {
      abort();
    }
  }
}

void NetworkNode::OnUvAsyncExit() {
  uv_stop(&loop_);
  running_ = false;
}

void NetworkNode::PutTask(const Task& task) {
  std::unique_lock<std::mutex> lock(mtx_);
  task_queue_.push(task);
}

bool NetworkNode::GetTask(Task* task) {
  std::unique_lock<std::mutex> lock(mtx_);
  if (task_queue_.empty()) {
    return false;
  }
  *task = task_queue_.front();
  task_queue_.pop();
  return true;
}

void NetworkNode::Notify() { uv_async_send(&async_task_req_); }

}  // namespace ft
