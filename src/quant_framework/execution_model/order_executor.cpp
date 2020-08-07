// Copyright[2020] < Copyright Kevin, kevin.lau.gd @gmail.com>

#include "execution_model/order_executor.h"

#include <thread>

namespace ft {

using std::literals::operator""s;

void OrderExecutor::start() {
  std::thread([this] {
    for (;;) {
      mutex_.lock();
      for (auto& model : models_) {
        model->execute();
      }
      std::this_thread::sleep_for(0.1s);
    }
  }).detach();
}

}  // namespace ft
