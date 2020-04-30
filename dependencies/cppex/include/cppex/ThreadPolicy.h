// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef CPPEX_INCLUDE_CPPEX_THREADPOLICY_H_
#define CPPEX_INCLUDE_CPPEX_THREADPOLICY_H_

/*
 * 用于单线程或是多线程的锁模型，单线程不加锁，多线程可选择
 * 对象级别的锁或是类级别的锁
 */

namespace cppex {

class SingleThreadModel {
 public:
  void lock() {}

  void unlock() {}
};

template<class Mutex>
class ObjectLevelModel {
 public:
  void lock() {
    mutex_.lock();
  }

  void unlock() {
    mutex_.unlock();
  }

 private:
  Mutex mutex_;
};

template<class Class, class Mutex>
class ClassLevelModel {
 public:
  void lock() {
    mutex_.lock();
  }

  void unlock() {
    mutex_.unlock();
  }

 private:
  static Mutex mutex_;
};

}

#endif  // CPPEX_INCLUDE_CPPEX_THREADPOLICY_H_
