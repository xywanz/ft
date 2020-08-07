// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "broker/ocg_connection.h"

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/timerfd.h>
#include <unistd.h>

#include <functional>

#include "broker/connection_manager.h"

namespace ft::bss {

static void set_nonblock(int fd) {
  int flags = 0;
  flags = fcntl(fd, F_GETFL);
  flags |= O_NONBLOCK;
  fcntl(fd, F_SETFL, flags);
}

static void set_block(int fd) {
  int flags = 0;
  flags = fcntl(fd, F_GETFL);
  flags &= ~O_NONBLOCK;
  fcntl(fd, F_SETFL, flags);
}

OcgConnection::OcgConnection(ConnectionManager* conn_mgr, Session* session,
                             const std::string& ip, uint16_t port)
    : conn_mgr_(conn_mgr), session_(session), ip_(ip), port_(port) {
  decoder_.set_handler(session_);
}

OcgConnection::~OcgConnection() {
  if (recv_thread_ && recv_thread_->joinable()) {
    recv_thread_->join();
  }

  if (sockfd_ >= 0) close(sockfd_);
  if (timerfd_ >= 0) close(timerfd_);

  printf("disconnect\n");
}

bool OcgConnection::connect() {
  sockfd_ = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd_ < 0) {
    printf("[OcgConnection::connect] failed to creat socket\n");
    return false;
  }

  sockaddr_in addrin{};
  if (inet_pton(AF_INET, ip_.c_str(), &addrin.sin_addr) <= 0) {
    printf("[OcgConnection::connect] failed to inet_pton\n");
    return false;
  }
  addrin.sin_family = AF_INET;
  addrin.sin_port = htons(static_cast<int16_t>(port_));

  // 下面是进行了一次异步的连接，如果60秒未连接成功，则主动断开连接
  set_nonblock(sockfd_);
  if (::connect(sockfd_, (sockaddr*)(&addrin), sizeof(addrin)) != 0) {
    if (errno == EINPROGRESS || errno == EAGAIN) {
      pollfd pfd{};
      pfd.fd = sockfd_;
      pfd.events = POLLOUT;
      int timeout = 60 * 1000;
      uint64_t start_ms = now_ms();
    restart:
      int res = poll(&pfd, 1, 60 * 1000);
      if (res == 0) {
        printf("[OcgConnection::connect] timeout\n");
        return false;
      } else if (res < 0) {
        if (res == EINTR) {
          uint64_t now = now_ms();
          timeout = 60 * 1000 - static_cast<int>(now - start_ms);
          if (timeout > 0) goto restart;
          printf("[OcgConnection::connect] timeout\n");
          return false;
        }
        printf("[OcgConnection::connect] unknown error\n");
        return false;
      } else {
        if (pfd.revents & (POLLERR | POLLHUP)) {
          printf("[OcgConnection::connect] failed to connect\n");
          return false;
        }
      }
    } else {  // 连接直接失败了
      printf("[OcgConnection::connect] failed to connect\n");
      return false;
    }
  }

  set_block(sockfd_);
  recv_thread_ = std::make_unique<std::thread>(
      std::mem_fn(&OcgConnection::recv_and_parse_data), this);
  return true;
}

bool OcgConnection::send(const void* buf, std::size_t size) {
  // just for test
  std::size_t total_send = 0;
  auto p = reinterpret_cast<const char*>(buf);

restart:
  while (total_send < size) {
    auto res = ::send(sockfd_, p + total_send, size - total_send, 0);
    if (res == 0) {
      session_->disconnect(DisconnectReason::SOCKET_ERROR, true);
      return false;
    } else if (res < 0) {
      if (errno == EINTR) goto restart;
      session_->disconnect(DisconnectReason::SOCKET_ERROR, true);
      return false;
    } else {
      total_send += res;
    }
  }

  return true;
}

void OcgConnection::disconnect(DisconnectReason reason) {
  if (timerfd_ >= 0) {
    close(timerfd_);
    timerfd_ = -1;
  }

  if (sockfd_ >= 0) {
    close(sockfd_);
    sockfd_ = -1;
    conn_mgr_->on_disconnect(reason);
  }
}

void OcgConnection::recv_and_parse_data() {
  session_->set_socket_sender(this);

  timerfd_ = timerfd_create(CLOCK_MONOTONIC, 0);
  if (timerfd_ < 0) {
    session_->disconnect(DisconnectReason::SOCKET_ERROR, false);
    return;
  }
  itimerspec ts{};
  ts.it_interval.tv_sec = 1;  // 一秒的定时器

  timespec tv;
  clock_gettime(CLOCK_REALTIME, &tv);
  // 第一次触发的时间设置成下一个一秒的开始
  ts.it_value.tv_nsec = 1000000000 - tv.tv_nsec;
  timerfd_settime(timerfd_, 0, &ts, nullptr);

  pollfd pfd[2]{};
  pfd[0].fd = sockfd_;
  pfd[0].events = POLLIN;
  pfd[1].fd = timerfd_;
  pfd[1].events = POLLIN;

  for (;;) {
    int nfds = poll(pfd, 2, -1);
    if (nfds == 0) {
      continue;
    } else if (nfds < 0) {
      if (errno == EINTR) continue;
      session_->disconnect(DisconnectReason::SOCKET_ERROR, false);
      return;
    }

    if (pfd[0].revents != 0) {
      auto res =
          recv(sockfd_, decoder_.writable_start(), decoder_.writable_size(), 0);
      if (res == 0) {
        session_->disconnect(DisconnectReason::SOCKET_ERROR, false);
        return;
      } else if (res < 0) {
        if (errno == EINTR) continue;
        session_->disconnect(DisconnectReason::SOCKET_ERROR, false);
        return;
      } else {
        decoder_.parse_raw_data(res);
      }
    }

    if (pfd[1].revents != 0) {
      uint64_t tmp;
      read(pfd[1].fd, &tmp, 8);
      if (!session_->periodically_check()) {
        return;
      }
    }
  }
}

}  // namespace ft::bss
