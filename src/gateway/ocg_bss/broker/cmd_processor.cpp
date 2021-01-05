// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "broker/cmd_processor.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cassert>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "broker/broker.h"

namespace ft::bss {

static int create_servfd(int port) {
  int servfd = socket(AF_INET, SOCK_STREAM, 0);
  assert(servfd > 0);

  int opt = 1;
  setsockopt(servfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(int));

  sockaddr_in addrin{};
  addrin.sin_family = AF_INET;
  addrin.sin_addr.s_addr = INADDR_ANY;
  addrin.sin_port = htons(port);

  int res = bind(servfd, reinterpret_cast<sockaddr*>(&addrin), sizeof(addrin));
  assert(res == 0);

  res = ::listen(servfd, 5);
  assert(res == 0);

  return servfd;
}

static int accept(int servfd) {
  sockaddr cliaddr{};
  socklen_t socklen = sizeof(cliaddr);
  int sockfd;

restart:
  sockfd = ::accept(servfd, &cliaddr, &socklen);
  if (sockfd < 0) {
    if (errno == EINTR) goto restart;
    abort();
  }
  sockfd = sockfd;
  return sockfd;
}

CmdProcessor::CmdProcessor(::ft::BssBroker* broker) : broker_(broker) { assert(broker); }

void CmdProcessor::start(int port) {
  const int bufsz = 4096 * 8;
  char buf[bufsz];
  size_t sz;
  int servfd = create_servfd(port);

  for (;;) {
    sz = 0;
    int sockfd = accept(servfd);

    for (;;) {
      auto n = recv(sockfd, buf + sz, bufsz - sz, 0);
      if (n == 0) break;
      if (n < 0) {
        if (errno == EINTR) break;
        continue;
      }
      sz += n;
      if (sz < sizeof(CmdHeader) || sz < reinterpret_cast<CmdHeader*>(buf)->length) continue;

      auto hdr = reinterpret_cast<CmdHeader*>(buf);
      if (!ProcessCmd(*hdr, reinterpret_cast<const char*>(hdr + 1))) break;

      sz -= hdr->length;
      memmove(buf, buf + hdr->length, sz);
    }

    close(sockfd);
  }
}

bool CmdProcessor::ProcessCmd(const CmdHeader& hdr, const char* body) {
  printf("cmd: <magic:0x%x, type:%u, length:%u>\n", hdr.magic, hdr.type, hdr.length);
  if (hdr.magic != kCommandMagic) return false;

  switch (hdr.type) {
    case BSS_CMD_LOGON: {
      process_logon(*reinterpret_cast<const LogonCmd*>(body));
      break;
    }
    case BSS_CMD_LOGOUT: {
      process_logout();
      break;
    }
    case BSS_CMD_NEW_ORDER: {
      process_new_order(*reinterpret_cast<const NewOrderCmd*>(body));
      break;
    }
    case BSS_CMD_AMEND_ORDER: {
      process_amend_order(*reinterpret_cast<const AmendOrderCmd*>(body));
      break;
    }
    case BSS_CMD_CANCEL_ORDER: {
      process_cancel_order(*reinterpret_cast<const CancelOrderCmd*>(body));
      break;
    }
    case BSS_CMD_MASS_CANCEL: {
      process_mass_cancel(*reinterpret_cast<const MassCancelCmd*>(body));
      break;
    }
    default: {
      return false;
    }
  }

  return true;
}

void CmdProcessor::process_logon(const LogonCmd& cmd) {
  broker_->logon(cmd.password, cmd.new_password);
}

void CmdProcessor::process_logout() { broker_->Logout(); }

void CmdProcessor::process_new_order(const NewOrderCmd& cmd) {}

void CmdProcessor::process_amend_order(const AmendOrderCmd& cmd) {}

void CmdProcessor::process_cancel_order(const CancelOrderCmd& cmd) {}

void CmdProcessor::process_mass_cancel(const MassCancelCmd& cmd) {}

}  // namespace ft::bss
