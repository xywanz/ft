// Copyright [2020-2021] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_INCLUDE_FT_COMPONENT_PUBSUB_SUBSCRIBER_H_
#define FT_INCLUDE_FT_COMPONENT_PUBSUB_SUBSCRIBER_H_

#include <ft/component/pubsub/serializable.h>
#include <ft/component/pubsub/socket.h>

#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>

namespace ft::pubsub {

class Subscriber {
 public:
  explicit Subscriber(const std::string& address);

  template <class T>
  bool Subscribe(const std::string& routing_key, std::function<void(T* data)> callback) {
    if (!sock_->Subscribe(routing_key)) {
      return false;
    }

    auto cb_handle = [&](const std::string& msg) {
      T data;
      if (!data.ParseFromString(msg)) {
        throw std::runtime_error("cannot deserialize from str");
      }
      callback(&data);
    };
    cb_.emplace(routing_key, cb_handle);

    return true;
  }

  void GetReply() {
    std::string topic, msg;
    if (!sock_->RecvMsg(&topic, &msg)) {
      throw std::runtime_error("subscriber: recv failed");
    }

    auto& cb = cb_.at(topic);
    cb(msg);
  }

  void Start();

 private:
  std::unique_ptr<SocketBase> sock_{nullptr};
  std::unordered_map<std::string, std::function<void(const std::string&)>> cb_;
};

}  // namespace ft::pubsub

#endif  // FT_INCLUDE_FT_COMPONENT_PUBSUB_SUBSCRIBER_H_
