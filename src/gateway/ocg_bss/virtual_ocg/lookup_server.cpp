// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "virtual_ocg/lookup_server.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cassert>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "virtual_ocg/ocg_encoder.h"
#include "virtual_ocg/ocg_parser.h"

using namespace ft::bss;

void LookupServer::listen(int port) {
  servfd_ = socket(AF_INET, SOCK_STREAM, 0);
  assert(servfd_ > 0);

  int opt = 1;
  setsockopt(servfd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(int));

  sockaddr_in addrin{};
  addrin.sin_family = AF_INET;
  addrin.sin_addr.s_addr = INADDR_ANY;
  addrin.sin_port = htons(port);

  int res = bind(servfd_, reinterpret_cast<sockaddr*>(&addrin), sizeof(addrin));
  assert(res == 0);

  res = ::listen(servfd_, 5);
  assert(res == 0);
  printf("start lookup server at 0.0.0.0:%d\n", port);
}

void LookupServer::run() {
  for (;;) {
    accept();
    process_request();
    close(sockfd_);
  }
}

void LookupServer::accept() {
  sockaddr cliaddr{};
  socklen_t socklen = sizeof(cliaddr);
restart:
  int sockfd = ::accept(servfd_, reinterpret_cast<sockaddr*>(&cliaddr), &socklen);
  if (sockfd < 0) {
    if (errno == EINTR) goto restart;
    abort();
  } else {
    sockfd_ = sockfd;
    printf("lookup server: bss connected\n");
  }
}

void LookupServer::process_request() {
  char buf[4096];
  uint32_t total_recv = 0;

  do {
    auto res = recv(sockfd_, buf, 4096 - total_recv, 0);
    if (res == 0 || (res < 0 && errno != EINTR)) {
      return;
    } else if (res > 0) {
      total_recv += res;
    }
  } while (total_recv < sizeof(MessageHeader) ||
           total_recv < reinterpret_cast<MessageHeader*>(buf)->length);

  auto header = reinterpret_cast<MessageHeader*>(buf);
  if (header->message_type != LOOKUP_REQUEST) {
    printf("lookup server: invalid message type\n");
    return;
  }

  LookupRequest req{};
  parse_lookup_request(*header, reinterpret_cast<char*>(header + 1), &req);
  if (req.protocol_type != 1 || req.type_of_service != 1) {
    printf("lookup server: invalid request\n");
    return;
  }

  LookupResponse rsp{};
  rsp.status = LOOKUP_SERVICE_ACCEPTED;
  strncpy(rsp.primary_ip, "0.0.0.0", sizeof(rsp.primary_ip));
  rsp.primary_port = 18765;
  strncpy(rsp.secondary_ip, "0.0.0.0", sizeof(rsp.secondary_ip));
  rsp.secondary_port = 18766;

  OcgEncoder encoder;
  encoder.set_comp_id(header->comp_id);

  MsgBuffer msgbuf;
  encoder.encode_msg(rsp, &msgbuf);
  ::send(sockfd_, msgbuf.data, msgbuf.size, 0);
  printf("lookup server: send response\n");
}
