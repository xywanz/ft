// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef OCG_BSS_BROKER_MISC_H_
#define OCG_BSS_BROKER_MISC_H_

#include <sys/timeb.h>

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>

#include "protocol/protocol.h"

namespace ft::bss {

#define BUG_ON()                                                 \
  do {                                                           \
    printf("BUG_ON %s:%d %s()\n", __FILE__, __LINE__, __func__); \
    fflush(stdout);                                              \
    abort();                                                     \
  } while (0)

inline uint64_t now_ms() {
  return std::chrono::time_point_cast<std::chrono::milliseconds>(
             std::chrono::steady_clock::now())
      .time_since_epoch()
      .count();
}

}  // namespace ft::bss

#endif  // OCG_BSS_BROKER_MISC_H_
