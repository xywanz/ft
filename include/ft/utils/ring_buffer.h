// Copyright [2020-2021] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_INCLUDE_FT_UTILS_RING_BUFFER_H_
#define FT_INCLUDE_FT_UTILS_RING_BUFFER_H_

#include <cstdint>
#include <limits>
#include <utility>
#include <vector>

#define STORE_BARRIER() __asm__ __volatile__("sfence" : : : "memory")
#define LOAD_BARRIER() __asm__ __volatile__("lfence" : : : "memory")
#define MEMORY_BARRIES() __asm__ __volatile__("mfence" : : : "memory")
#define COMPILER_BARRIES() __asm__ __volatile__("" : : : "memory")

namespace ft {

constexpr inline bool IsPowerOfTwo(uint32_t val) {
  if (val == 0) {
    return false;
  }

  return ((val - 1) & val) == 0;
}

// 单读单写的循环队列
template <class T, uint32_t N>
class RingBuffer {
  static_assert(IsPowerOfTwo(N));

 public:
  RingBuffer() { data_.resize(N); }

  void Enqueue(T&& data) {
    while (size() == N) {
      continue;
    }

    data_[write_seq_ & kIdxMask] = std::move(data);
    STORE_BARRIER();
    ++write_seq_;
  }

  void Enqueue(const T& data) {
    while (size() == N) {
      continue;
    }

    data_[write_seq_ & kIdxMask] = data;
    STORE_BARRIER();
    ++write_seq_;
  }

  void Dequeue(T* p) {
    while (empty()) {
      continue;
    }

    if (p) {
      *p = data_[read_seq_ & kIdxMask];
    }
    STORE_BARRIER();
    ++read_seq_;
  }

  uint32_t size() const { return write_seq_ - (std::numeric_limits<uint32_t>::max() + read_seq_); }

  uint32_t empty() const { return write_seq_ == read_seq_; }

  uint32_t capacity() const { return N; }

 private:
  std::vector<T> data_;
  volatile uint32_t read_seq_ [[gnu::aligned(64)]] = 0;
  volatile uint32_t write_seq_ [[gnu::aligned(64)]] = 0;
  constexpr static uint32_t kIdxMask = N - 1;
};

}  // namespace ft

#endif  // FT_INCLUDE_FT_UTILS_RING_BUFFER_H_
