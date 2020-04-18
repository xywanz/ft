
// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_INCLUDE_EVENTENGINE_H_
#define FT_INCLUDE_EVENTENGINE_H_

#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <utility>

#include <cppex/Any.h>

#include "Account.h"
#include "Contract.h"
#include "Common.h"
#include "LoginParams.h"
#include "MarketData.h"
#include "Order.h"
#include "Position.h"
#include "Trade.h"

namespace ft {

enum EventType : int {
  EV_TICK = 0,
  EV_ORDER,
  EV_TRADE,
  EV_POSITION,
  EV_ACCOUNT,
  EV_CONTRACT,
  EV_USER_EVENT_START
};

#define MEM_HANDLER(f) std::bind(std::mem_fn(&f), this, std::placeholders::_1)

class EventEngine {
 public:
  using HandleType = std::function<void(cppex::Any*)>;

  virtual ~EventEngine() {}

  template<class T>
  void post(int event, T* ctx) {
    {
      std::unique_lock<std::mutex> lock(mutex_);
      event_queue_.emplace(event, ctx);
    }
    cv_.notify_one();
  }

  void post(int event) {
    {
      std::unique_lock<std::mutex> lock(mutex_);
      event_queue_.emplace(event);
    }
    cv_.notify_one();
  }

  bool set_handler(int event, HandleType&& handler) {
    if (event < 0 || event >= kMaxHandlers)
      return false;
    handlers_[event] = std::move(handler);
  }

  void run(bool loop_in_this_thread = true) {
    if (loop_in_this_thread) {
      loop();
    } else {
      std::thread([this] { loop(); }).detach();
    }
  }

 private:
  struct Event {
    template<class T>
    Event(int _event, T* _data)
      : event(_event),
        data(_data) {
    }

    explicit Event(int _event)
      : event(_event),
        data() {
    }

    int event;
    cppex::Any data;
  };

  void process_event(Event* ev) {
    int event = ev->event;
    if (event < 0 || event >= kMaxHandlers || !handlers_[event])
      return;

    handlers_[event](&ev->data);
  }

  void loop() {
    for (;;) {
      std::unique_lock<std::mutex> lock(mutex_);
      cv_.wait(lock, [this] { return !event_queue_.empty(); });
      auto ev = std::move(event_queue_.front());
      event_queue_.pop();
      lock.unlock();
      process_event(&ev);
    }
  }

  inline static const std::size_t kMaxHandlers = 256;
  HandleType handlers_[kMaxHandlers];

  std::queue<Event> event_queue_;
  std::mutex mutex_;
  std::condition_variable cv_;
};

}  // namespace ft

#endif  // FT_INCLUDE_EVENTENGINE_H_
