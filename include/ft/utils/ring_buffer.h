// Copyright [2020-2021] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_INCLUDE_FT_UTILS_RING_BUFFER_H_
#define FT_INCLUDE_FT_UTILS_RING_BUFFER_H_

#include <atomic>
#include <utility>

namespace ft {

template <typename T, uint64_t capacity>
class RingBuffer {
 public:
  template <typename U>
  bool Put(U&& data) {
    uint64_t cur_head = head_.load(std::memory_order::memory_order_relaxed);
    if (cur_head - tail_.load(std::memory_order::memory_order_acquire) == capacity) {
      return false;
    }
    buffer_[cur_head & kIndexMask_] = std::forward<U>(data);
    head_.store(cur_head + 1, std::memory_order::memory_order_release);
    return true;
  }

  template <class U>
  void PutWithBlocking(U&& data) {
    bool status;
    do {
      status = Put(std::forward<U>(data));
    } while (!status);
  }

  bool Get(T* p) {
    uint64_t cur_tail = tail_.load(std::memory_order::memory_order_relaxed);
    if (cur_tail == head_.load(std::memory_order::memory_order_acquire)) {
      return false;
    }
    *p = std::move(buffer_[cur_tail & kIndexMask_]);
    tail_.store(cur_tail + 1, std::memory_order::memory_order_release);
    return true;
  }

  void GetWithBlocking(T* p) {
    bool status;
    do {
      status = Get(p);
    } while (!status);
  }

  uint64_t available_read_size() const {
    return head_.load(std::memory_order::memory_order_acquire) -
           tail_.load(std::memory_order::memory_order_relaxed);
  }

  uint64_t available_write_size() const {
    return capacity - (head_.load(std::memory_order::memory_order_acquire) -
                       tail_.load(std::memory_order::memory_order_relaxed));
  }

  bool is_empty() const { return available_read_size() == 0; }

  bool is_full() const { return available_write_size() == 0; }

 private:
  static constexpr uint64_t kIndexMask_ = capacity - 1;

  alignas(64) std::atomic<uint64_t> head_{0};
  alignas(64) std::atomic<uint64_t> tail_{0};
  alignas(64) T buffer_[capacity]{};

  static_assert(capacity != 0 && (capacity & (capacity - 1)) == 0, "capacity must be power of 2");
};

}  // namespace ft

#endif  // FT_INCLUDE_FT_UTILS_RING_BUFFER_H_
