// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "broker/connection_manager.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include "broker/constants.h"

namespace ft::bss {

ConnectionManager::ConnectionManager(Session* session) : session_(session) {
  for (auto& address : lookup_servers_) {
    address.first = "0.0.0.0";
    address.second = 18888;
  }
}

void ConnectionManager::on_disconnect(DisconnectReason reason) {
  should_disconnect_ = true;
  disconnect_reason_ = reason;
}

void ConnectionManager::run() {
  // 因session主动close，或是OCG主动close，都应该间隔一定时间才进行重连
  // 重连原则：
  // * 如果连接突然断开(主动断或是被动断)或是收到OCG发来的Logout，间隔10秒再重试
  // * 如果发出Logon后没有60秒内没收到任何回应，间隔60秒再重试
  // * 多次connect失败应该重新查找lookup service获取新的ocg地址
  request_ocg_address();

  for (;;) {
    // 如果连接不上需要等待10秒再重试
    if (!do_connect_to_ocg(true)) {
      sleep(RECONNECT_INTERVAL_SEC);

      // 尝试连接备用服务器
      if (!do_connect_to_ocg(false)) {
        sleep(RECONNECT_INTERVAL_SEC);

        // 仍然连接不上，请求lookup services
        request_ocg_address();
        continue;
      }
    }

    while (!should_disconnect_) sleep(1);
    ocg_conn_.reset();

    // 如果是对方没有给予登录回应，应该60秒后再尝试重连
    if (disconnect_reason_ == DisconnectReason::LOGON_TIMEOUT) {
      sleep(RECONNECT_INTERVAL_SEC_IF_NOT_RSP);
    } else {
      sleep(RECONNECT_INTERVAL_SEC);
    }
  }
}

/*
 * 连接lookup server以查询OCG的地址
 *
 * lookup server共有4个地址，并按以下顺序进行轮询查找，直至查询成功
 *   1. Primary site primary Lookup Service
 *   2. Primary site mirror Lookup Service
 *   3. Backup site primary Lookup Service
 *   4. Backup site mirror Lookup Service
 * 如果查询失败（连接中断或查询被拒），则下一次查询要间隔5秒
 */
void ConnectionManager::request_ocg_address() {
  for (const auto& [ip, port] : lookup_servers_) {
    int sockfd = connect_to_lookup_server(ip, port);
    if (sockfd < 0) {
      sleep(LOOKUP_SERVICE_RETRY_INTERVAL_SEC);
      continue;
    }

    // 生成查询请求
    if (!send_lookup_request(sockfd)) {
      close(sockfd);
      sleep(LOOKUP_SERVICE_RETRY_INTERVAL_SEC);
      continue;
    }

    // 接收并解析查询请求
    LookupResponse rsp{};
    if (!recv_lookup_response(sockfd, &rsp)) {
      close(sockfd);
      sleep(LOOKUP_SERVICE_RETRY_INTERVAL_SEC);
      continue;
    }
    close(sockfd);

    // 收到正确的请求
    primary_ocg_address_.first = rsp.primary_ip;
    primary_ocg_address_.second = rsp.primary_port;
    secondary_ocg_address_.first = rsp.secondary_ip;
    secondary_ocg_address_.second = rsp.secondary_port;
    printf("lookup response. primary: %s:%u  secondary: %s:%u\n",
           primary_ocg_address_.first.c_str(), primary_ocg_address_.second,
           secondary_ocg_address_.first.c_str(), secondary_ocg_address_.second);
    return;
  }
}

int ConnectionManager::connect_to_lookup_server(const std::string& ip, uint16_t port) {
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) {
    printf("request_ocg_address. failed to creat socket\n");
    abort();
  }

  sockaddr_in addrin{};
  if (inet_pton(AF_INET, ip.c_str(), &addrin.sin_addr) <= 0) {
    printf("request_ocg_address. failed to inet_pton\n");
    abort();
  }
  addrin.sin_family = AF_INET;
  addrin.sin_port = htons(static_cast<int16_t>(port));

  if (::connect(sockfd, (sockaddr*)(&addrin), sizeof(addrin)) != 0) {
    printf("request_ocg_address. failed to connect\n");
    close(sockfd);
    return -1;
  }

  return sockfd;
}

bool ConnectionManager::send_lookup_request(int sockfd) {
  BinaryMessageEncoder encoder;
  MsgBuffer msg_buf;
  LookupRequest lookup_req{1, 1};

  encoder.set_comp_id(session_->comp_id());
  encoder.encode_msg(lookup_req, &msg_buf);
  // 发送查询请求
  uint32_t total_send = 0;
  do {
    auto res = send(sockfd, msg_buf.data, msg_buf.size, 0);
    if (res == 0 || (res < 0 && errno != EINTR)) {
      printf("failed to send lookup request\n");
      return false;
    } else if (res > 0) {
      total_send += res;
    }
  } while (total_send != msg_buf.size);

  return true;
}

bool ConnectionManager::recv_lookup_response(int sockfd, LookupResponse* rsp) {
  char buf[4096];
  uint32_t total_recv = 0;

  do {
    auto res = recv(sockfd, buf, 4096 - total_recv, 0);
    if (res == 0 || (res < 0 && errno != EINTR)) {
      return false;
    } else if (res > 0) {
      total_recv += res;
    }
  } while (total_recv < sizeof(MessageHeader) ||
           total_recv < reinterpret_cast<MessageHeader*>(buf)->length);

  auto header = reinterpret_cast<MessageHeader*>(buf);
  if (header->comp_id != session_->comp_id() || header->message_type != LOOKUP_RESPONSE) {
    printf("invalid lookup response\n");
    return false;
  }

  parse_lookup_response(*header, reinterpret_cast<char*>(header + 1), rsp);
  if (rsp->status != LOOKUP_SERVICE_ACCEPTED) {
    printf("request_ocg_address. rejected: reason:%u\n", rsp->lookup_reject_code);
    close(sockfd);
    sleep(LOOKUP_SERVICE_RETRY_INTERVAL_SEC);
    return false;
  }

  return true;
}

bool ConnectionManager::do_connect_to_ocg(bool use_primary) {
  // 如果检测到当前状态不允许登录，则每隔一秒重新检测一次
  while (!session_->is_enabled()) sleep(1);

  should_disconnect_ = false;
  if (use_primary) {
    ocg_conn_ = std::make_unique<OcgConnection>(this, session_, primary_ocg_address_.first,
                                                primary_ocg_address_.second);
  } else {
    ocg_conn_ = std::make_unique<OcgConnection>(this, session_, secondary_ocg_address_.first,
                                                secondary_ocg_address_.second);
  }

  if (!ocg_conn_->connect()) {
    ocg_conn_.reset();
    return false;
  }

  return true;
}

}  // namespace ft::bss
