// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef OCG_BSS_BROKER_SOCKET_SENDER_H_
#define OCG_BSS_BROKER_SOCKET_SENDER_H_

#include "protocol/protocol_encoder.h"

namespace ft::bss {

enum class DisconnectReason {
  LOGON_TIMEOUT = 100,
  LOGOUT_TIMEOUT,
  INCORRECT_MSG,
  LOGOUT,
  TEST_REQUEST_FAILED,
  SOCKET_ERROR,
};

class SocketSender {
 public:
  virtual ~SocketSender() {}

  virtual bool send(const void* buf, std::size_t size) = 0;

  virtual void disconnect(DisconnectReason reason) = 0;
};

}  // namespace ft::bss

#endif  // OCG_BSS_COUNTER_SOCKET_SENDER_H_
