// Copyright [2020-present] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_INCLUDE_FT_UTILS_REDIS_H_
#define FT_INCLUDE_FT_UTILS_REDIS_H_

#include "ft/component/trader_db.h"

#include <cassert>
#include <memory>
#include <string>
#include <vector>

#include "fmt/format.h"
#include "ft/base/log.h"
#include "hiredis.h"

namespace ft {

class RedisSession {
 private:
  struct RedisReplyDestructor {
    void operator()(redisReply* p) {
      if (p) {
        freeReplyObject(p);
      }
    }
  };

 public:
  using RedisReply = std::shared_ptr<redisReply>;

 public:
  RedisSession() {}

  ~RedisSession() {
    if (ctx_) {
      redisFree(ctx_);
    }
  }

  bool Connect(const std::string& ip, int port) {
    ctx_ = redisConnect(ip.c_str(), port);
    if (!ctx_) {
      return false;
    }
    if (ctx_->err != 0) {
      redisFree(ctx_);
      ctx_ = nullptr;
      return false;
    }
    return true;
  }

  bool Set(const std::string& key, const void* p, size_t size) {
    const char* argv[3];
    size_t argvlen[3];

    argv[0] = "set";
    argvlen[0] = 3;

    argv[1] = key.c_str();
    argvlen[1] = key.length();

    argv[2] = reinterpret_cast<const char*>(p);
    argvlen[2] = size;

    auto* reply = reinterpret_cast<redisReply*>(redisCommandArgv(ctx_, 3, argv, argvlen));
    if (reply) {
      freeReplyObject(reply);
      return true;
    }
    return false;
  }

  RedisReply Get(const std::string& key) const {
    const char* argv[2];
    size_t argvlen[2];

    argv[0] = "get";
    argvlen[0] = 3;

    argv[1] = key.c_str();
    argvlen[1] = key.length();

    auto* reply = reinterpret_cast<redisReply*>(redisCommandArgv(ctx_, 2, argv, argvlen));
    return RedisReply(reply, RedisReplyDestructor());
  }

  RedisReply Keys(const std::string& pattern) const {
    const char* argv[2];
    size_t argvlen[2];

    argv[0] = "keys";
    argvlen[0] = 4;

    argv[1] = pattern.c_str();
    argvlen[1] = pattern.length();

    auto* reply = reinterpret_cast<redisReply*>(redisCommandArgv(ctx_, 2, argv, argvlen));
    return RedisReply(reply, RedisReplyDestructor());
  }

  bool Del(const std::string& key) {
    const char* argv[2];
    size_t argvlen[2];

    argv[0] = "del";
    argvlen[0] = 3;

    argv[1] = key.c_str();
    argvlen[1] = key.length();

    auto* reply = reinterpret_cast<redisReply*>(redisCommandArgv(ctx_, 2, argv, argvlen));
    if (reply) {
      freeReplyObject(reply);
      return true;
    }
    return false;
  }

 private:
  redisContext* ctx_ = nullptr;
};

class TraderDBImpl {
 public:
  explicit TraderDBImpl(std::unique_ptr<RedisSession>&& redis_session)
      : redis_session_(std::move(redis_session)) {}

  bool GetPosition(const std::string& strategy, const std::string& ticker, Position* res) {
    auto reply = redis_session_->Get(GetKey(strategy, ticker));
    if (!reply) {
      return false;
    }
    if (reply->str) {
      *res = *reinterpret_cast<Position*>(reply->str);
    } else {
      *res = Position{};
    }
    return true;
  }

  bool GetAllPositions(const std::string& strategy, std::vector<Position>* res) {
    auto pattern = fmt::format("pos/{}/*", strategy);
    auto keys_reply = redis_session_->Keys(pattern);
    if (!keys_reply) {
      return false;
    }
    for (size_t i = 0; i < keys_reply->elements; ++i) {
      auto get_reply = redis_session_->Get(keys_reply->element[i]->str);
      if (!get_reply) {
        return false;
      }
      res->emplace_back(*reinterpret_cast<Position*>(get_reply->str));
    }
    return true;
  }

  bool SetPosition(const std::string& strategy, const std::string& ticker, const Position& pos) {
    return redis_session_->Set(GetKey(strategy, ticker), &pos, sizeof(pos));
  }

  bool ClearPositions(const std::string& strategy) {
    auto pattern = fmt::format("pos/{}/*", strategy);
    auto keys_reply = redis_session_->Keys(pattern);
    if (!keys_reply) {
      return false;
    }
    for (size_t i = 0; i < keys_reply->elements; ++i) {
      if (!redis_session_->Del(keys_reply->element[i]->str)) {
        return false;
      }
    }
    return true;
  }

  std::string GetKey(const std::string& strategy, const std::string& ticker) const {
    return fmt::format("pos/{}/{}", strategy, ticker);
  }

 private:
  std::unique_ptr<RedisSession> redis_session_;
};

TraderDB::TraderDB() {}

TraderDB::~TraderDB() {
  if (db_impl_) {
    delete reinterpret_cast<TraderDBImpl*>(db_impl_);
  }
}

bool TraderDB::Init(const std::string& address, const std::string& username,
                    const std::string& password) {
  auto delim_pos = address.find_first_of(':');
  if (delim_pos == std::string::npos || delim_pos == 0 || delim_pos == address.size() - 1) {
    LOG_ERROR("[TraderDB::Init] invalid TraderDB address {}", address);
    return false;
  }
  auto ip = address.substr(0, delim_pos);
  int port = -1;
  try {
    port = std::stoi(address.substr(delim_pos + 1));
  } catch (...) {
    LOG_ERROR("[TraderDB::Init] invalid port. address:{}", address);
    return false;
  }

  auto redis_session = std::make_unique<RedisSession>();
  if (!redis_session->Connect(ip, port)) {
    LOG_ERROR("[TraderDB::Init] failed to open db connection. address:{}", address);
    return false;
  }
  auto* db_impl = new TraderDBImpl(std::move(redis_session));
  db_impl_ = db_impl;
  return true;
}

bool TraderDB::GetPosition(const std::string& strategy, const std::string& ticker,
                           Position* res) const {
  if (!db_impl_) {
    return false;
  }
  return reinterpret_cast<TraderDBImpl*>(db_impl_)->GetPosition(strategy, ticker, res);
}

bool TraderDB::GetAllPositions(const std::string& strategy, std::vector<Position>* res) const {
  if (!db_impl_) {
    LOG_ERROR("[TraderDB::GetAllPositions] failed");
    return false;
  }
  return reinterpret_cast<TraderDBImpl*>(db_impl_)->GetAllPositions(strategy, res);
}

bool TraderDB::SetPosition(const std::string& strategy, const std::string& ticker,
                           const Position& pos) {
  if (!db_impl_) {
    LOG_ERROR("[TraderDB::SetPosition] failed");
    return false;
  }
  return reinterpret_cast<TraderDBImpl*>(db_impl_)->SetPosition(strategy, ticker, pos);
}

bool TraderDB::ClearPositions(const std::string& strategy) {
  if (!db_impl_) {
    LOG_ERROR("[TraderDB::ClearPositions] failed");
    return false;
  }
  return reinterpret_cast<TraderDBImpl*>(db_impl_)->ClearPositions(strategy);
}

}  // namespace ft

#endif  // FT_INCLUDE_FT_UTILS_REDIS_H_
