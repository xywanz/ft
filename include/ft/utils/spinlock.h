// Copyright [2020-2021] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_INCLUDE_FT_UTILS_SPINLOCK_H_
#define FT_INCLUDE_FT_UTILS_SPINLOCK_H_

#include <atomic>

namespace ft {

class SpinLock {
 public:
  SpinLock() = default;
  SpinLock(const SpinLock&) = delete;
  SpinLock& operator=(const SpinLock&) = delete;

  void lock() {
    while (flag_.test_and_set(std::memory_order_acquire)) {
      continue;
    }
  }

  void unlock() { flag_.clear(std::memory_order_release); }

 private:
  std::atomic_flag flag_ = ATOMIC_FLAG_INIT;
};

}  // namespace ft

#endif  // FT_INCLUDE_FT_UTILS_SPINLOCK_H_
