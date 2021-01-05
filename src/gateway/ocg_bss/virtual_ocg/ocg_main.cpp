// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include <string>
#include <thread>

#include "virtual_ocg/lookup_server.h"
#include "virtual_ocg/virtual_ocg.h"

using namespace ft::bss;

int main(int argc, char** argv) {
  VirtualOcg ocg;
  ocg.Init();
  ocg.listen(18765, 18766);

  std::thread lookup_thread([] {
    LookupServer lookup_server;
    lookup_server.listen(18888);
    lookup_server.run();
  });

  for (;;) {
    ocg.accept();
    ocg.run();
  }
}
