// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef BSS_BROKER_CONNECTION_MANAGER_H_
#define BSS_BROKER_CONNECTION_MANAGER_H_

#include <memory>

#include "broker/ocg_connection.h"
#include "broker/session.h"

namespace ft::bss {

class ConnectionManager {
 public:
  explicit ConnectionManager(Session* session);

  void on_disconnect(DisconnectReason reason);

  void run();

 private:
  void request_ocg_address();

  int connect_to_lookup_server(const std::string& ip, uint16_t port);

  bool send_lookup_request(int sockfd);

  bool recv_lookup_response(int sockfd, LookupResponse* rsp);

  bool do_connect_to_ocg(bool use_primary = true);

 private:
  using Address = std::pair<std::string, uint16_t>;

  std::array<Address, 4> lookup_servers_;
  Address primary_ocg_address_;
  Address secondary_ocg_address_;

  Session* session_;
  std::unique_ptr<OcgConnection> ocg_conn_;
  volatile bool should_disconnect_ = false;
  DisconnectReason disconnect_reason_;
};

}  // namespace ft::bss

#endif  // BSS_BROKER_CONNECTION_MANAGER_H_
