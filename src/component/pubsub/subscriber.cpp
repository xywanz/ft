// Copyright [2020-2021] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "ft/component/pubsub/subscriber.h"

namespace ft::pubsub {

Subscriber::Subscriber(const std::string& address) {
  // init socket
  abort();
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
