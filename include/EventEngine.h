
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

namespace ft {

#define MEM_HANDLER(f) std::bind(std::mem_fn(&f), this, std::placeholders::_1)

class EventEngine {
 public:
  using HandleType = std::function<void(cppex::Any*)>;

  ~EventEngine() {
    stop();
  }

  template<class T>
  void post(int event, T* ctx) {
    {
      std::unique_lock<std::mutex> lock(mutex_);
      if (!is_to_stop_)
        event_queue_.emplace(event, ctx);
    }
    cv_.notify_one();
  }

  void post(int event) {
    {
      std::unique_lock<std::mutex> lock(mutex_);
      if (!is_to_stop_)
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
    is_to_stop_ = false;
    is_stopped_ = false;

    if (loop_in_this_thread) {
      loop();
    } else {
      std::thread([this] { loop(); }).detach();
    }
  }

  void stop() {
    is_to_stop_ = true;
    cv_.notify_one();
    while (!is_stopped_)
      continue;
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

    if (ev->data.empty())
      handlers_[event](nullptr);
    else
      handlers_[event](&ev->data);
  }

  void loop() {
    std::queue<Event> tmp;
    std::unique_lock<std::mutex> lock(mutex_, std::defer_lock);
    while (!is_to_stop_) {
      lock.lock();
      cv_.wait(lock, [this] { return !event_queue_.empty() || is_to_stop_; });
      tmp.swap(event_queue_);
      lock.unlock();

      while (!tmp.empty()) {
        auto ev = std::move(tmp.front());
        tmp.pop();
        process_event(&ev);
      }
    }

    is_stopped_ = true;
  }

 private:
  inline static const std::size_t kMaxHandlers = 256;
  HandleType handlers_[kMaxHandlers];

  std::queue<Event> event_queue_;
  std::mutex mutex_;
  std::condition_variable cv_;

  volatile bool is_to_stop_ = false;
  volatile bool is_stopped_ = false;
};

}  // namespace ft

#endif  // FT_INCLUDE_EVENTENGINE_H_
