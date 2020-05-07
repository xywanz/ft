#include <hiredis.h>

int main() {
  auto* c = redisConnect("127.0.0.1", 6379);
  if (!c || c->err)
    exit(-1);

  auto* reply = (redisReply*)redisCommand(c, "SET %s %d", "a", 1);
  freeReplyObject(reply);

  reply = (redisReply*)redisCommand(c, "GET %s", "a");
  printf("%s\n", reply->str);
}
