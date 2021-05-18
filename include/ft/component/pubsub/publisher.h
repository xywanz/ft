// Copyright [2020-2021] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_INCLUDE_FT_COMPONENT_PUBSUB_PUBLISHER_H_
#define FT_INCLUDE_FT_COMPONENT_PUBSUB_PUBLISHER_H_

#include <memory>
#include <string>

#include "ft/component/pubsub/socket.h"
#include "ft/component/serializable.h"

namespace ft::pubsub {

class Publisher {
 public:
  explicit Publisher(const std::string& address);

  template <class T>
  bool Publish(const std::string& routing_key, const Serializable<T>& data) {
    std::string msg;
    data.SerializeToString(&msg);
    return sock_->SendMsg(routing_key, msg);
  }

 private:
  std::unique_ptr<SocketBase> sock_{nullptr};
};

}  // namespace ft::pubsub

#endif  // FT_INCLUDE_FT_COMPONENT_PUBSUB_PUBLISHER_H_
