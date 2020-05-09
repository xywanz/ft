// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_INCLUDE_IPC_REDIS_H_
#define FT_INCLUDE_IPC_REDIS_H_

#include <async.h>
#include <fmt/format.h>
#include <hiredis.h>

#include <cassert>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

namespace ft {

struct RedisReplyDestructor {
  void operator()(redisReply* p) { freeReplyObject(p); }
};

class RedisSession {
 public:
  RedisSession(const std::string& ip, int port) {
    ctx_ = redisConnect(ip.c_str(), port);
    assert(ctx_ && ctx_->err == 0);
  }

  void set(const std::string& key, const void* p, size_t size) {
    const char* argv[3];
    size_t argvlen[3];

    argv[0] = "set";
    argvlen[0] = 3;

    argv[1] = key.c_str();
    argvlen[1] = key.length();

    argv[2] = reinterpret_cast<const char*>(p);
    argvlen[2] = size;

    auto* reply =
        reinterpret_cast<redisReply*>(redisCommandArgv(ctx_, 3, argv, argvlen));
    assert(reply);
    freeReplyObject(reply);
  }

  std::unique_ptr<redisReply, RedisReplyDestructor> get(
      const std::string& key) const {
    const char* argv[2];
    size_t argvlen[2];

    argv[0] = "get";
    argvlen[0] = 3;

    argv[1] = key.c_str();
    argvlen[1] = key.length();

    auto* reply =
        reinterpret_cast<redisReply*>(redisCommandArgv(ctx_, 2, argv, argvlen));
    assert(reply);
    return std::unique_ptr<redisReply, RedisReplyDestructor>(reply);
  }

  std::unique_ptr<redisReply, RedisReplyDestructor> keys(
      const std::string& pattern) const {
    const char* argv[2];
    size_t argvlen[2];

    argv[0] = "keys";
    argvlen[0] = 4;

    argv[1] = pattern.c_str();
    argvlen[1] = pattern.length();

    auto* reply =
        reinterpret_cast<redisReply*>(redisCommandArgv(ctx_, 2, argv, argvlen));
    assert(reply);
    return std::unique_ptr<redisReply, RedisReplyDestructor>(reply);
  }

  void subscribe(const std::vector<std::string>& topics) {
    if (topics.empty()) return;

    std::stringstream ss;
    std::string args;
    for (const auto& topic : topics) ss << topic << " ";
    ss >> args;

    auto* reply = reinterpret_cast<redisReply*>(
        redisCommand(ctx_, "subscribe %s", args.c_str()));
    freeReplyObject(reply);
  }

  std::unique_ptr<redisReply, RedisReplyDestructor> get_sub_reply() {
    redisReply* reply;
    auto status = redisGetReply(ctx_, reinterpret_cast<void**>(&reply));
    assert(status == REDIS_OK);
    return std::unique_ptr<redisReply, RedisReplyDestructor>(reply);
  }

  void publish(const std::string& topic, const void* p, size_t size) {
    const char* argv[3];
    size_t argvlen[3];

    argv[0] = "publish";
    argvlen[0] = 7;

    argv[1] = topic.c_str();
    argvlen[1] = topic.length();

    argv[2] = reinterpret_cast<const char*>(p);
    argvlen[2] = size;

    auto* reply =
        reinterpret_cast<redisReply*>(redisCommandArgv(ctx_, 3, argv, argvlen));
    assert(reply);
    freeReplyObject(reply);
  }

 private:
  redisContext* ctx_;
};

class AsyncRedisSession {
 public:
  AsyncRedisSession(const std::string& ip, int port) {
    ctx_ = redisAsyncConnect(ip.c_str(), port);
    assert(ctx_);
  }

  void set(const std::string& key, const void* p, size_t size,
           redisCallbackFn* cb, void* privdata = nullptr) {
    const char* argv[3];
    size_t argvlen[3];

    argv[0] = "set";
    argvlen[0] = 3;

    argv[1] = key.c_str();
    argvlen[1] = key.length();

    argv[2] = reinterpret_cast<const char*>(p);
    argvlen[2] = size;

    auto status = redisAsyncCommandArgv(ctx_, cb, privdata, 3, argv, argvlen);
    assert(status == REDIS_OK);
  }

  void get(const std::string& key, redisCallbackFn* cb,
           void* privdata = nullptr) const {
    const char* argv[2];
    size_t argvlen[2];

    argv[0] = "get";
    argvlen[0] = 3;

    argv[1] = key.c_str();
    argvlen[1] = key.length();

    auto status = redisAsyncCommandArgv(ctx_, cb, privdata, 2, argv, argvlen);
    assert(status == REDIS_OK);
  }

  void keys(const std::string& pattern, redisCallbackFn* cb,
            void* privdata = nullptr) const {
    const char* argv[2];
    size_t argvlen[2];

    argv[0] = "keys";
    argvlen[0] = 4;

    argv[1] = pattern.c_str();
    argvlen[1] = pattern.length();

    auto status = redisAsyncCommandArgv(ctx_, cb, privdata, 2, argv, argvlen);
    assert(status == REDIS_OK);
  }

 private:
  redisAsyncContext* ctx_;
};

}  // namespace ft

#endif  // FT_INCLUDE_IPC_REDIS_H_
