// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include <memory>

#include "ft/strategy/algo_order/target_pos_engine.h"
#include "ft/strategy/strategy.h"
#include "spdlog/spdlog.h"

class TargetPosDemo : public ft::Strategy {
 public:
  void OnInit() override {
    Subscribe({ticker_});
    target_pos_engine_ =
        std::make_unique<ft::TargetPosEngine>(ft::ContractTable::get_by_ticker(ticker_)->ticker_id);
    RegisterAlgoOrderEngine(target_pos_engine_.get());
  }

  void OnTick(const ft::TickData& tick) override {
    target_pos_engine_->SetTargetPos(8);  // 设置仓位为8
  }

 private:
  std::string ticker_ = "rb2105";
  std::unique_ptr<ft::TargetPosEngine> target_pos_engine_;
};

EXPORT_STRATEGY(TargetPosDemo);
