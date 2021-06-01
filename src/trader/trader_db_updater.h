// Copyright [2020-2021] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_SRC_TRADER_TRADER_DB_UPDATER_H_
#define FT_SRC_TRADER_TRADER_DB_UPDATER_H_

#include <atomic>
#include <string>
#include <thread>

#include "ft/base/log.h"
#include "ft/component/trader_db.h"
#include "ft/utils/ring_buffer.h"

namespace ft {

class TraderDBUpdater {
 private:
  struct PositionContext {
    std::string strategy;
    std::string ticker;
    Position pos;
  };

 public:
  ~TraderDBUpdater() {
    running_ = false;
    if (wr_thread_.joinable()) {
      wr_thread_.join();
    }
  }

  bool Init(const std::string& address, const std::string& username, const std::string& password) {
    if (!trader_db_.Init(address, username, password)) {
      LOG_ERROR("[TraderDBUpdater::Init] failed to open db connection");
      return false;
    }

    running_ = true;
    wr_thread_ = std::thread([this] {
      PositionContext ctx;
      while (running_) {
        while (pos_rb_.Get(&ctx)) {
          if (!trader_db_.SetPosition(ctx.strategy, ctx.ticker, ctx.pos)) {
            LOG_ERROR("TraderDBUpdater. failed to update position");
            return;
          }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
      }
    });
    return true;
  }

  bool SetPosition(const std::string& strategy, const std::string& ticker, const Position& pos) {
    PositionContext ctx;
    ctx.strategy = strategy;
    ctx.ticker = ticker;
    ctx.pos = pos;
    pos_rb_.PutWithBlocking(ctx);
    return true;
  }

  TraderDB* GetTraderDB() { return &trader_db_; }

 private:
  TraderDB trader_db_;
  std::thread wr_thread_;
  std::atomic<bool> running_ = false;
  RingBuffer<PositionContext, 1024> pos_rb_;
};

}  // namespace ft

#endif  // FT_SRC_TRADER_TRADER_DB_UPDATER_H_
