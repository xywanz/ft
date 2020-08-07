// Copyright[2020] < Copyright Kevin, kevin.lau.gd @gmail.com>

#ifndef FT_SRC_QUANT_FRAMEWORK_EXECUTION_MODEL_SIMPLE_TWAP_H_
#define FT_SRC_QUANT_FRAMEWORK_EXECUTION_MODEL_SIMPLE_TWAP_H_

#include "core/protocol.h"
#include "execution_model/execution_model.h"

namespace ft {

class SimpleTwap : public ExecutionModel {
 public:
  SimpleTwap(const TraderOrderReq& req, int volume_limit, int total_time_sec,
             int time_interval_sec);

  int timer_period() override;
  bool execute() override;
  void on_order_rsp(const OrderResponse& rsp) override;
  void terminate() override;

 private:
  TraderOrderReq req_;
  int volume_limit_;
  uint64_t total_time_;
  uint64_t time_interval_;
  uint64_t start_time_sec;
  uint64_t last_time_sec;
  int total_traded_{0};
};

}  // namespace ft

#endif  // FT_SRC_QUANT_FRAMEWORK_EXECUTION_MODEL_SIMPLE_TWAP_H_
