// Copyright [2020-2021] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "ft/utils/redis.h"

namespace ft {

RedisSession::RedisSession(bool nonblock) : RedisSession("127.0.0.1", 6379, nonblock) {}

RedisSession::RedisSession(const std::string& ip, int port, bool nonblock) {
  if (nonblock)
    ctx_ = redisConnectNonBlock(ip.c_str(), port);
  else
    ctx_ = redisConnect(ip.c_str(), port);
  assert(ctx_ && ctx_->err == 0);
}

void RedisSession::set_timeout(uint64_t timeout_ms) {
  timeval tv;
  tv.tv_sec = timeout_ms / 1000;
  tv.tv_usec = (timeout_ms % 1000) * 1000;

  redisSetTimeout(ctx_, tv);
}

void RedisSession::set(const std::string& key, const void* p, size_t size) {
  const char* argv[3];
  size_t argvlen[3];

  argv[0] = "set";
  argvlen[0] = 3;

  argv[1] = key.c_str();
  argvlen[1] = key.length();

  argv[2] = reinterpret_cast<const char*>(p);
  argvlen[2] = size;

  auto* reply = reinterpret_cast<redisReply*>(redisCommandArgv(ctx_, 3, argv, argvlen));
  assert(reply);
  freeReplyObject(reply);
}

RedisReply RedisSession::get(const std::string& key) const {
  const char* argv[2];
  size_t argvlen[2];

  argv[0] = "get";
  argvlen[0] = 3;

  argv[1] = key.c_str();
  argvlen[1] = key.length();

  auto* reply = reinterpret_cast<redisReply*>(redisCommandArgv(ctx_, 2, argv, argvlen));
  if (!reply) return nullptr;

  return RedisReply(reply, RedisReplyDestructor());
}

RedisReply RedisSession::keys(const std::string& pattern) const {
  const char* argv[2];
  size_t argvlen[2];

  argv[0] = "keys";
  argvlen[0] = 4;

  argv[1] = pattern.c_str();
  argvlen[1] = pattern.length();

  auto* reply = reinterpret_cast<redisReply*>(redisCommandArgv(ctx_, 2, argv, argvlen));
  assert(reply);
  return RedisReply(reply, RedisReplyDestructor());
}

void RedisSession::del(const std::string& key) {
  const char* argv[2];
  size_t argvlen[2];

  argv[0] = "del";
  argvlen[0] = 3;

  argv[1] = key.c_str();
  argvlen[1] = key.length();

  auto* reply = reinterpret_cast<redisReply*>(redisCommandArgv(ctx_, 2, argv, argvlen));
  freeReplyObject(reply);
}

void RedisSession::subscribe(const std::vector<std::string>& topics) {
  if (topics.empty()) return;

  for (const auto& topic : topics) {
    auto* reply = reinterpret_cast<redisReply*>(redisCommand(ctx_, "subscribe %s", topic.c_str()));
    freeReplyObject(reply);
  }
}

void RedisSession::publish(const std::string& topic, const void* p, size_t size) {
  const char* argv[3];
  size_t argvlen[3];

  argv[0] = "publish";
  argvlen[0] = 7;

  argv[1] = topic.c_str();
  argvlen[1] = topic.length();

  argv[2] = reinterpret_cast<const char*>(p);
  argvlen[2] = size;

  auto* reply = reinterpret_cast<redisReply*>(redisCommandArgv(ctx_, 3, argv, argvlen));
  assert(reply);
  freeReplyObject(reply);
}

RedisReply RedisSession::get_sub_reply() {
  redisReply* reply;
  auto status = redisGetReply(ctx_, reinterpret_cast<void**>(&reply));
  if (status == REDIS_OK) return RedisReply(reply, RedisReplyDestructor());
  return nullptr;
}

}  // namespace ft
