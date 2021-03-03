// Copyright [2020-2021] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_INCLUDE_FT_COMPONENT_PUBSUB_SOCKET_H_
#define FT_INCLUDE_FT_COMPONENT_PUBSUB_SOCKET_H_

#include <string>

namespace ft::pubsub {

class SocketBase {
 public:
  virtual ~SocketBase() {}
  virtual bool Connect(const std::string& address) = 0;
  virtual bool Bind(const std::string& address) = 0;
  virtual bool Subscribe(const std::string& topic) = 0;
  virtual bool SendMsg(const std::string& topic, const std::string& msg) = 0;
  virtual bool RecvMsg(std::string* topic, std::string* msg) = 0;
};

}  // namespace ft::pubsub

#endif  // FT_INCLUDE_FT_COMPONENT_PUBSUB_SOCKET_H_
