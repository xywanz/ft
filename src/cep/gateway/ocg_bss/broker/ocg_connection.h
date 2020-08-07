// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef BSS_BROKER_OCG_CONNECTION_H_
#define BSS_BROKER_OCG_CONNECTION_H_

#include <memory>
#include <mutex>
#include <string>
#include <thread>

#include "broker/session.h"
#include "broker/socket_sender.h"
#include "protocol/protocol_parser.h"

namespace ft::bss {

class ConnectionManager;

class OcgConnection : public SocketSender {
 public:
  OcgConnection(ConnectionManager* conn_mgr, Session* session,
                const std::string& ip, uint16_t port);

  ~OcgConnection();

  bool connect();

  bool send(const void* buf, std::size_t size) override;

  void disconnect(DisconnectReason reason) override;

 private:
  void recv_and_parse_data();

 private:
  ConnectionManager* conn_mgr_;
  Session* session_;
  BinaryMessageDecoder decoder_;
  std::unique_ptr<std::thread> recv_thread_;

  int sockfd_ = -1;
  int timerfd_ = -1;
  std::string ip_;
  uint16_t port_;
};

}  // namespace ft::bss

#endif  // BSS_BROKER_OCG_CONNECTION_H_
