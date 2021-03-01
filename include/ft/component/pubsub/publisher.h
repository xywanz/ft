// Copyright [2020-2021] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_INCLUDE_FT_COMPONENT_PUBSUB_PUBLISHER_H_
#define FT_INCLUDE_FT_COMPONENT_PUBSUB_PUBLISHER_H_

#include <ft/component/pubsub/serializable.h>
#include <ft/component/pubsub/socket.h>

#include <memory>
#include <string>

namespace ft::pubsub {

class Publisher {
 public:
  explicit Publisher(const std::string& address);

  template <class T>
  bool Publish(const std::string& routing_key, const Serializable<T>& data) {
    std::string msg;
    if (!data.SerializeToString(&msg)) {
      return false;
    }
    if (!sock_->SendMsg(routing_key, msg)) {
      return false;
    }

    return true;
  }

 private:
  std::unique_ptr<SocketBase> sock_{nullptr};
};

}  // namespace ft::pubsub

#endif  // FT_INCLUDE_FT_COMPONENT_PUBSUB_PUBLISHER_H_
