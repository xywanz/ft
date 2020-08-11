// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "cep/rms/common/strategy_notifier.h"

namespace ft {

void StrategyNotifier::on_order_accepted(const Order* order) {
  if (order->strategy_id[0] != 0) {
    OrderResponse rsp{};
    rsp.client_order_id = order->client_order_id;
    rsp.order_id = order->req.order_id;
    rsp.tid = order->req.contract->tid;
    rsp.direction = order->req.direction;
    rsp.offset = order->req.offset;
    rsp.original_volume = order->req.volume;
    rsp.error_code = NO_ERROR;
    rsp_redis_.publish(order->strategy_id, &rsp, sizeof(rsp));
  }
}

void StrategyNotifier::on_order_traded(const Order* order, const Trade* trade) {
  if (order->strategy_id[0] != 0) {
    OrderResponse rsp{};
    rsp.client_order_id = order->client_order_id;
    rsp.order_id = order->req.order_id;
    rsp.tid = order->req.contract->tid;
    rsp.direction = order->req.direction;
    rsp.offset = order->req.offset;
    rsp.original_volume = order->req.volume;
    rsp.traded_volume = order->traded_volume;
    rsp.this_traded = trade->volume;
    rsp.this_traded_price = trade->price;
    rsp.completed =
        order->canceled_volume + order->traded_volume == order->req.volume;
    rsp.error_code = NO_ERROR;
    rsp_redis_.publish(order->strategy_id, &rsp, sizeof(rsp));
  }
}

void StrategyNotifier::on_order_canceled(const Order* order, int canceled) {
  Trade tmp{};
  on_order_traded(order, &tmp);
}

void StrategyNotifier::on_order_rejected(const Order* order, int error_code) {
  if (order->strategy_id[0] != 0) {
    OrderResponse rsp{};
    rsp.client_order_id = order->client_order_id;
    rsp.order_id = order->req.order_id;
    rsp.tid = order->req.contract->tid;
    rsp.direction = order->req.direction;
    rsp.offset = order->req.offset;
    rsp.original_volume = order->req.volume;
    rsp.completed = true;
    rsp.error_code = error_code;
    rsp_redis_.publish(order->strategy_id, &rsp, sizeof(rsp));
  }
}

}  // namespace ft
