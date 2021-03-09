// Copyright [2020-2021] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_INCLUDE_FT_STRATEGY_ALGO_ORDER_TARGET_POS_ENGINE_H_
#define FT_INCLUDE_FT_STRATEGY_ALGO_ORDER_TARGET_POS_ENGINE_H_

#include <map>
#include <string>

#include "ft/strategy/algo_order/algo_order_engine.h"
#include "ft/utils/protocol_utils.h"

namespace ft {

// TargetPosEngine用于自动调整至设定的目标仓位
// 适用于不擅长处理订单回报的策略程序
// 用法:
//    策略init函数中
//    void OnInit() {
//      target_pos_engine_ = std::make_unique<TargetPosEngine>(ticker_id);
//      RegisterAlgoOrderEngine(target_pos_engine_.get());
//    }
//   需要调整仓位的逻辑处
//      target_pos_engine_->SetTargetPos(xx);
class TargetPosEngine : public AlgoOrderEngine {
 public:
  explicit TargetPosEngine(int ticker_id);

  void Init() override;

  // 调整下单价格幅度，不设置时默认为0
  // 多：ask_1 + price_scope
  // 空：bid_1 - price_scope
  void SetPriceLimit(double price_limit);

  // 设置目标仓位，正数表示多，负数表示空
  void SetTargetPos(int volume);

  void OnTick(const TickData& tick) override;

  void OnOrder(const OrderResponse& order) override;

  void OnTrade(const OrderResponse& trade) override;

 private:
  void SendOrder(Direction direction, Offset offset, int volume, double price) {
    AlgoOrderEngine::SendOrder(ticker_id_, volume, direction, offset, OrderType::kLimit, price,
                               client_order_id_);
    orders_.emplace(client_order_id_, PendingOrder{0, direction, offset, ask_, volume});
    ++client_order_id_;
  }

 private:
  struct PendingOrder {
    uint64_t order_id;
    Direction direction;
    Offset offset;
    double price;
    int volume;
  };

 private:
  uint32_t ticker_id_;
  std::string ticker_;
  uint32_t client_order_id_;

  int target_pos_ = 0;

  int long_pos_ = 0;
  int short_pos_ = 0;
  int long_pending_ = 0;
  int short_pending_ = 0;

  double price_limit_ = 0.0;

  double bid_ = 0.0;
  double ask_ = 0.0;

  std::map<uint32_t, PendingOrder> orders_;
};

}  // namespace ft

#endif  // FT_INCLUDE_FT_STRATEGY_ALGO_ORDER_TARGET_POS_ENGINE_H_
