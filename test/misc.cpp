#include <unistd.h>

#include <thread>

#include "IPC/redis.h"

struct A {
  int a = 100;
};

int main() {
  ft::RedisSession sess("127.0.0.1", 6379);

  std::thread([]() {
    ft::RedisSession sess("127.0.0.1", 6379);
    while (true) {
      A a;
      a.a = 188;
      sess.publish("a", &a, sizeof(A));
      sleep(1);
    }
  }).detach();

  sess.subscribe({"a"});
  redisReply* reply;
  while (true) {
    auto reply = sess.get_sub_reply();
    A* a = (A*)reply->element[2]->str;
    fmt::print("{}: {}\n", (char*)reply->element[1]->str, a->a);
  }
}
