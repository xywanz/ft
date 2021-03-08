// Copyright [2020-2021] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include <memory>

#include "ft/strategy/algo_order/target_pos_engine.h"
#include "ft/strategy/strategy.h"
#include "spdlog/spdlog.h"

class SpreadArb : public ft::Strategy {
 public:
  void OnInit() override {
    auto* near_contract = ft::ContractTable::get_by_ticker(near_ticker_);
    auto* deferred_contract = ft::ContractTable::get_by_ticker(deferred_ticker_);

    if (!near_contract || !deferred_contract) {
      spdlog::error("contract not found: {} {}", near_ticker_, deferred_ticker_);
      exit(1);
    }

    near_ticker_id_ = near_contract->ticker_id;
    deferred_ticker_id_ = deferred_contract->ticker_id;

    near_target_pos_ = std::make_unique<ft::TargetPosEngine>(near_ticker_id_);
    deferred_target_pos_ = std::make_unique<ft::TargetPosEngine>(deferred_ticker_id_);
    RegisterAlgoOrderEngine(near_target_pos_.get());
    RegisterAlgoOrderEngine(deferred_target_pos_.get());

    Subscribe({near_ticker_, deferred_ticker_});
  }

  void OnTick(const ft::TickData& tick) override {
    if (tick.last_price > 0.0) {
      if (tick.ticker_id == near_ticker_id_) {
        near_quote_ = tick.last_price;
      } else if (tick.ticker_id == deferred_ticker_id_) {
        deferred_quote_ = tick.last_price;
      }
    }

    if (near_quote_ > 0.0 && deferred_quote_ > 0.0) {
      double spread = near_quote_ - deferred_quote_;
      spdlog::info("spread: {}", spread);
      if (spread > 250.0) {
        near_target_pos_->SetTargetPos(-1);
        deferred_target_pos_->SetTargetPos(1);
      } else if (spread < 200.0) {
        near_target_pos_->SetTargetPos(0);
        deferred_target_pos_->SetTargetPos(0);
      }
    }
  }

 private:
  std::string near_ticker_ = "rb2104";
  std::string deferred_ticker_ = "rb2105";
  uint32_t near_ticker_id_;
  uint32_t deferred_ticker_id_;

  double near_quote_ = 0.0;
  double deferred_quote_ = 0.0;

  std::unique_ptr<ft::TargetPosEngine> near_target_pos_;
  std::unique_ptr<ft::TargetPosEngine> deferred_target_pos_;
};

EXPORT_STRATEGY(SpreadArb);
