// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef CPPEX_INCLUDE_CPPEX_ANY_H_
#define CPPEX_INCLUDE_CPPEX_ANY_H_

#include <memory>
#include <utility>

namespace cppex {

class Any {
 public:
  Any() {}

  template<class T>
  Any(T* ptr)
    : holder_(new PlaceHolder<T>(ptr)) {
  }

  template<class T>
  Any& operator=(T* ptr) {
    holder_.reset(new PlaceHolder<T>(ptr));
  }

  bool empty() const {
    return !holder_;
  }

  void release() {
    holder_->release();
    holder_.reset();
  }

  template<class T>
  std::unique_ptr<T> fetch() {
    if (holder_)
      return std::move(static_cast<PlaceHolder<T>*>(holder_.get())->holder);
    return nullptr;
  }

  template<class T>
  T* cast() {
    if (holder_)
      return static_cast<PlaceHolder<T>*>(holder_.get())->holder.get();
    return nullptr;
  }

  template<class T>
  const T* cast() const {
    return const_cast<Any*>(this)->cast<T>();
  }

 private:
  struct HolderBase {
    virtual ~HolderBase() {}
    virtual void release() {}
  };

  template<class T>
  struct PlaceHolder : public HolderBase {
    explicit PlaceHolder(T* ptr)
      : holder(ptr) {
    }

    void release() override {
      holder.release();
    }

    std::unique_ptr<T> holder = nullptr;
  };

  std::unique_ptr<HolderBase> holder_ = nullptr;
};

}  // namespace cppex

#endif  // CPPEX_INCLUDE_CPPEX_ANY_H_
