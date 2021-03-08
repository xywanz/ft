// Copyright [2020-2021] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_SRC_COMPONENT_PUBSUB_ZMQ_SOCKET_H_
#define FT_SRC_COMPONENT_PUBSUB_ZMQ_SOCKET_H_

#include <memory>
#include <string>

#include "ft/component/pubsub/socket.h"
#include "zmq.hpp"
#include "zmq_addon.hpp"

namespace ft::pubsub {

class ZmqSocket : public SocketBase {
 public:
  bool Connect(const std::string& address) override;
  bool Bind(const std::string& address) override;
  bool Subscribe(const std::string& topic) override;
  bool SendMsg(const std::string& topic, const std::string& msg) override;
  bool RecvMsg(std::string* topic, std::string* msg) override;

 private:
  std::unique_ptr<zmq::context_t> zmq_ctx_{nullptr};
  std::unique_ptr<zmq::socket_t> zmq_sock_{nullptr};
};

}  // namespace ft::pubsub

#endif  // FT_SRC_COMPONENT_PUBSUB_ZMQ_SOCKET_H_
