// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef BSS_TEST_LOOKUP_SERVER_H_
#define BSS_TEST_LOOKUP_SERVER_H_

class LookupServer {
 public:
  void listen(int port);
  void run();

 private:
  void accept();
  void process_request();

 private:
  int servfd_{-1};
  int sockfd_{-1};
};

#endif  // BSS_TEST_LOOKUP_SERVER_H_
