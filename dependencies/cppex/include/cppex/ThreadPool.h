// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef CPPEX_INCLUDE_CPPEX_THREADPOOL_H_
#define CPPEX_INCLUDE_CPPEX_THREADPOOL_H_

#include <condition_variable>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <utility>
#include <vector>

namespace cppex {

class ThreadPool {
 public:
  explicit ThreadPool(std::size_t n) {
    for (std::size_t i = 0; i < n; ++i) {
      thread_pool_.emplace_back([this]() {
        while (true) {
          std::function<void()> task;
          {
            std::unique_lock<std::mutex> lock(mutex_);
            cv_.wait(lock, [this] { return !task_queue_.empty(); });
            task = std::move(this->task_queue_.front());
            task_queue_.pop();
          }
          task();
        }
      });
    }
  }

  template<class F, class... Args>
  auto exuecute(F&& f, Args&&... args)
        -> std::future<typename std::result_of<F(Args...)>::type> {
    using ResultType = typename std::result_of<F(Args...)>::type;

    auto task = std::make_shared<std::packaged_task<ResultType()>>(
                    std::bind(std::forward<F>(f), std::forward<Args>(args)...));
    auto ret = task->get_future();
    {
      std::unique_lock<std::mutex> lock(mutex_);
      task_queue_.emplace([task] { (*task)(); });
    }
    cv_.notify_one();
    return ret;
  }

  template<class... Args>
  void exuecute(std::function<void(Args...)>&& f, Args&&... args) {
    auto task = std::make_shared<std::packaged_task<void()>>(
                    std::bind(std::forward<std::function<void(Args...)>>(f),
                              std::forward<Args>(args)...));
    {
      std::unique_lock<std::mutex> lock(mutex_);
      task_queue_.emplace([task] { (*task)(); });
    }
    cv_.notify_one();
  }

 private:
  std::vector<std::thread> thread_pool_;
  std::queue<std::function<void()>> task_queue_;
  std::mutex mutex_;
  std::condition_variable cv_;
};

}  // namespace cppex

#endif  // CPPEX_INCLUDE_CPPEX_THREADPOOL_H_
