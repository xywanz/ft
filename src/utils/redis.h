// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_SRC_UTILS_REDIS_H_
#define FT_SRC_UTILS_REDIS_H_

#include <async.h>
#include <hiredis.h>

#include <cassert>
#include <memory>
#include <string>
#include <vector>

#include "utils/misc.h"

namespace ft {

using RedisReply = std::shared_ptr<redisReply>;

struct RedisReplyDestructor {
  void operator()(redisReply* p) {
    if (p) freeReplyObject(p);
  }
};

class RedisSession {
 public:
  explicit RedisSession(bool nonblock = false);
  RedisSession(const std::string& ip, int port, bool nonblock = false);

  void set_timeout(uint64_t timeout_ms);

  void set(const std::string& key, const void* p, size_t size);
  RedisReply get(const std::string& key) const;
  RedisReply keys(const std::string& pattern) const;
  void del(const std::string& key);
  void subscribe(const std::vector<std::string>& topics);
  void publish(const std::string& topic, const void* p, size_t size);

  RedisReply get_sub_reply();

 private:
  redisContext* ctx_ = nullptr;
};

}  // namespace ft

#endif  // FT_SRC_UTILS_REDIS_H_
