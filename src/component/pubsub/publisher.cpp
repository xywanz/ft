// Copyright [2020-2021] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "ft/component/pubsub/publisher.h"

#include <stdexcept>

#include "component/pubsub/zmq_socket.h"

namespace ft::pubsub {

Publisher::Publisher(const std::string& address) {
  sock_ = std::make_unique<ZmqSocket>();
  if (!sock_->Bind(address)) {
    throw std::runtime_error("publisher: cannot bind address");
  }
}

}  // namespace ft::pubsub
