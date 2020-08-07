// Copyright[2020] < Copyright Kevin, kevin.lau.gd @gmail.com>

#include "execution_model/simple_twap.h"

namespace ft {

SimpleTwap::SimpleTwap(const TraderOrderReq& req, int volume_limit,
                       int total_time_sec, int time_interval_sec)
    : req_(req),
      total_time_(total_time_sec),
      timer_period_(time_interval_sec) {}

int SimpleTwap::timer_period() { return timer_period_; }

bool SimpleTwap::execute() {
  
}

void SimpleTwap::on_order_rsp(const OrderResponse& rsp) {}

void SimpleTwap::terminate() {}

}  // namespace ft
