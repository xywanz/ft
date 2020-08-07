// Copyright[2020] < Copyright Kevin, kevin.lau.gd @gmail.com>

#ifndef FT_SRC_QUANT_FRAMEWORK_EXECUTION_MODEL_EXECUTION_MODEL_H_
#define FT_SRC_QUANT_FRAMEWORK_EXECUTION_MODEL_EXECUTION_MODEL_H_

#include <cstdint>

namespace ft {

class ExecutionModel {
 public:
  virtual ~ExecutionModel() {}

  virtual int timer_period() = 0;
  virtual bool execute() = 0;
  virtual void on_order_rsp(const OrderResponse& rsp) = 0;
  virtual void terminate() = 0;
};

}  // namespace ft

#endif  // FT_SRC_QUANT_FRAMEWORK_EXECUTION_MODEL_EXECUTION_MODEL_H_
