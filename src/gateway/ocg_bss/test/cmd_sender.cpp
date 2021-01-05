// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include <arpa/inet.h>
#include <getopt.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstdlib>
#include <cstring>
#include <string>

#include "broker/command.h"

using namespace ft::bss;

char kIp[32];
int kPort;
char kPasswd[9];
char kNewPasswd[9];

void parse_cmd(int argc, char** argv);
void Usage();
int connect(const std::string& ip, int port);
void send_cmd(const CmdHeader* hdr, const char* body);
void logon(int argc, char** argv);
void Logout(int argc, char** argv);

int main(int argc, char** argv) {
  if (argc < 2) {
    printf("please indicate op\n");
    Usage();
  }

  parse_cmd(argc - 1, argv + 1);

  if (strcmp(argv[1], "logon") == 0) {
    logon(argc - 1, argv + 1);
  } else if (strcmp(argv[1], "Logout") == 0) {
    Logout(argc - 1, argv + 1);
  } else {
    printf("please input correct op\n");
    Usage();
  }
}

void parse_cmd(int argc, char** argv) {
  option options[] = {
      {"addr", required_argument, 0, 0},
      {"passwd", optional_argument, 0, 0},
      {"new-passwd", optional_argument, 0, 0},
      {"help", optional_argument, 0, 0},
      {0, 0, 0, 0},
  };

  int opt, i;
  while ((opt = getopt_long(argc, argv, "", options, &i)) != -1) {
    if (opt != 0) Usage();

    if (strcmp(options[i].name, "addr") == 0) {
      try {
        int n = sscanf(optarg, "%[^:]:%d", kIp, &kPort);
        if (n != 2) {
          printf("please input correct ip:port\n");
          Usage();
        }
      } catch (...) {
        printf("please input correct ip:port\n");
        Usage();
      }
    } else if (strcmp(options[i].name, "passwd") == 0) {
      if (strlen(optarg) != 8) {
        printf("please input correct password(length=8)\n");
        Usage();
      }
      strncpy(kPasswd, optarg, sizeof(kPasswd));
    } else if (strcmp(options[i].name, "new-passwd") == 0) {
      if (strlen(optarg) != 8) {
        printf("please input correct new password(length=8)\n");
        Usage();
      }
      strncpy(kNewPasswd, optarg, sizeof(kNewPasswd));
    } else if (strcmp(options[i].name, "help") == 0) {
      Usage();
    }
  }

  if (kIp[0] == 0) {
    printf("please indicate ip:port\n");
    Usage();
  }
}

void Usage() { exit(-1); }

int connect(const std::string& ip, int port) {
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) abort();

  sockaddr_in addrin{};
  if (inet_pton(AF_INET, ip.c_str(), &addrin.sin_addr) <= 0) abort();

  addrin.sin_family = AF_INET;
  addrin.sin_port = htons(static_cast<int16_t>(port));

  if (::connect(sockfd, (sockaddr*)(&addrin), sizeof(addrin)) != 0) {
    close(sockfd);
    return -1;
  }

  return sockfd;
}

void send_cmd(int sockfd, const CmdHeader* hdr, const char* body) {
  char buf[4096];

  memcpy(buf, hdr, sizeof(*hdr));
  if (body) memcpy(buf + sizeof(*hdr), body, hdr->length - sizeof(*hdr));

  std::size_t sent = 0;
  while (sent < hdr->length) {
    auto n = ::send(sockfd, buf + sent, hdr->length - sent, 0);
    if (n == 0) exit(-1);
    if (n < 0) {
      if (errno == EINTR) continue;
      exit(-1);
    }
    sent += n;
  }

  printf("success\n");
}

void logon(int argc, char** argv) {
  CmdHeader hdr{};
  LogonCmd body{};
  hdr.magic = kCommandMagic;
  hdr.type = BSS_CMD_LOGON;
  hdr.length = sizeof(hdr) + sizeof(LogonCmd);

  strncpy(body.password, kPasswd, sizeof(body.password));
  strncpy(body.new_password, kNewPasswd, sizeof(body.new_password));

  int sockfd = connect(kIp, kPort);
  send_cmd(sockfd, &hdr, reinterpret_cast<const char*>(&body));
  close(sockfd);
}

void Logout(int argc, char** argv) {
  CmdHeader hdr{};
  hdr.magic = kCommandMagic;
  hdr.type = BSS_CMD_LOGOUT;
  hdr.length = sizeof(hdr);

  int sockfd = connect(kIp, kPort);
  send_cmd(sockfd, &hdr, nullptr);
  close(sockfd);
}
