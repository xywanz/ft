// Copyright [2020-2021] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "ft/component/pubsub/subscriber.h"

#include "component/pubsub/zmq_socket.h"

namespace ft::pubsub {

Subscriber::Subscriber(const std::string& address) {
  sock_ = std::make_unique<ZmqSocket>();
  if (!sock_->Connect(address)) {
    throw std::runtime_error("subscriber: cannot connect to address");
  }
}

void Subscriber::Start() {
  std::string topic, msg;
  for (;;) {
    if (!sock_->RecvMsg(&topic, &msg)) {
      throw std::runtime_error("subscriber: recv failed");
    }

    auto& cb = cb_.at(topic);
    cb(msg);
  }
}

}  // namespace ft::pubsub
