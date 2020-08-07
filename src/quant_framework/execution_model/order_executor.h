// Copyright[2020] < Copyright Kevin, kevin.lau.gd @gmail.com>

#ifndef FT_SRC_QUANT_FRAMEWORK_EXECUTION_MODEL_ORDER_EXECUTOR_H_
#define FT_SRC_QUANT_FRAMEWORK_EXECUTION_MODEL_ORDER_EXECUTOR_H_

#include <list>
#include <memory>
#include <mutex>

#include "core/protocol.h"
#include "execution_model/execution_model.h"

namespace ft {

class OrderExecutor {
 public:
  void start();

  int create_simple_twap(const TraderOrderReq& req, int volume_limit,
                         uint64_t total_time_sec, uint64_t time_interval_sec);

 private:
  std::mutex mutex_;
  std::list<std::unique_ptr<ExecutionModel>> models_;
};

}  // namespace ft

#endif  // FT_SRC_QUANT_FRAMEWORK_EXECUTION_MODEL_ORDER_EXECUTOR_H_
