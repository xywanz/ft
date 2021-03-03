// Copyright [2020-2021] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_INCLUDE_FT_UTILS_TIMER_THREAD_H_
#define FT_INCLUDE_FT_UTILS_TIMER_THREAD_H_

#include <atomic>
#include <condition_variable>
#include <functional>
#include <limits>
#include <map>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <utility>

namespace ft {

class TimerThread {
 private:
  struct TimerTask {
    int id;
    bool once;
    uint64_t interval_ms;
    std::function<bool()> cb;
  };

 public:
  ~TimerThread() { Stop(); }

  template <class F, class... Args>
  int AddTask(uint64_t interval_ms, uint64_t next_interval_ms, bool once, F&& f, Args&&... args) {
    int task_id = next_id_++;

    TimerTask task{};
    task.id = task_id;
    task.once = once;
    task.interval_ms = interval_ms;
    task.cb = std::bind(std::forward<F>(f), std::forward<Args>(args)...);

    uint64_t current_ms = GetCurrentTimeMS();
    CheckOverflow(current_ms, next_interval_ms);
    uint64_t expired_time_ms = GetCurrentTimeMS() + next_interval_ms;

    std::unique_lock<std::mutex> lock(mutex_);
    task_queue_.emplace(expired_time_ms, std::move(task));
    cv_.notify_one();
    return task_id;
  }

  template <class F, class... Args>
  int AddTask(uint64_t interval_ms, F&& f, Args&&... args) {
    return AddTask(interval_ms, interval_ms, false, std::forward<F>(f),
                   std::forward<Args>(args)...);
  }

  template <class F, class... Args>
  int AddTask(uint64_t interval_ms, bool once, F&& f, Args&&... args) {
    return AddTask(interval_ms, interval_ms, once, std::forward<F>(f), std::forward<Args>(args)...);
  }

  void RemoveTask(int task_id) {
    std::unique_lock<std::mutex> lock(mutex_);

    for (auto& pair : task_queue_) {
      auto range = task_queue_.equal_range(pair.first);
      for (auto iter = range.first; iter != range.second; ++iter) {
        if (iter->second.id == task_id) {
          task_queue_.erase(iter);
          return;
        }
      }
    }
  }

  void Start() {
    if (!timer_thread_) {
      running_ = true;
      timer_thread_ = std::make_unique<std::thread>(std::mem_fn(&TimerThread::MainLoop), this);
    }
  }

  void Stop() {
    if (timer_thread_) {
      running_ = false;
      cv_.notify_one();
      Join();
      timer_thread_.reset();
      task_queue_.clear();
    }
  }

  void Join() {
    if (timer_thread_ && timer_thread_->joinable()) {
      timer_thread_->join();
    }
  }

 private:
  uint64_t GetCurrentTimeMS() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::steady_clock::now().time_since_epoch())

        .count();
  }

  bool IsOverflow(uint64_t current_ms, uint64_t interval_ms) {
    return std::numeric_limits<uint64_t>::max() - current_ms < interval_ms;
  }

  void CheckOverflow(uint64_t current_ms, uint64_t interval_ms) {
    if (IsOverflow(current_ms, interval_ms)) {
      throw std::runtime_error("time overflow");
    }
  }

  void MainLoop() {
    std::chrono::milliseconds idle_wait_time(100000000UL);
    std::chrono::milliseconds wait_time(idle_wait_time);
    std::unique_lock<std::mutex> lock(mutex_);
    while (running_) {
      uint64_t current_ms = GetCurrentTimeMS();

      while (!task_queue_.empty()) {
        auto iter = task_queue_.begin();
        auto expired_time_ms = iter->first;

        if (current_ms < expired_time_ms) {
          break;
        }

        auto range = task_queue_.equal_range(expired_time_ms);
        auto& task = range.first->second;
        bool task_res = task.cb();
        if (!task_res || task.once) {
          task_queue_.erase(range.first);
        } else {
          CheckOverflow(expired_time_ms, task.interval_ms);
          uint64_t next_expired_time = expired_time_ms + task.interval_ms;
          auto new_task = std::move(task);
          task_queue_.erase(range.first);
          task_queue_.emplace(next_expired_time, new_task);
        }
      }

      if (!task_queue_.empty()) {
        uint64_t earlyest = task_queue_.begin()->first;
        current_ms = GetCurrentTimeMS();

        if (current_ms >= earlyest) {
          wait_time = std::chrono::milliseconds(0);
        } else {
          wait_time = std::chrono::milliseconds(earlyest - current_ms);
        }
      } else {
        wait_time = idle_wait_time;
      }

      cv_.wait_for(lock, std::chrono::milliseconds(wait_time));
    }
  }

 private:
  std::atomic<int> next_id_{0};
  std::unique_ptr<std::thread> timer_thread_;
  std::mutex mutex_;
  std::condition_variable cv_;
  volatile bool running_;

  std::multimap<uint64_t, TimerTask> task_queue_;
};

}  // namespace ft

#endif  // FT_INCLUDE_FT_UTILS_TIMER_THREAD_H_
