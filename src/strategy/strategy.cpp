// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "ft/strategy/strategy.h"

#include <thread>

#include "ft/component/yijinjing/journal/Timer.h"
#include "spdlog/spdlog.h"

namespace ft {

Strategy::Strategy() : md_sub_("ipc://md.ft_trader.ipc") {
  std::this_thread::sleep_for(std::chrono::milliseconds(200));  // 等待zmq连接建立
}

bool Strategy::Init(const StrategyConfig& config) {
  rsp_reader_ = yijinjing::JournalReader::create(".", config.rsp_mq_name, yijinjing::getNanoTime(),
                                                 config.strategy_name);
  sender_.Init(config.trade_mq_name);
  return true;
}

void Strategy::Run() {
  OnInit();
  if (backtest_mode_) {
    SendNotification(0);
  }

  std::thread trade_msg_thread([this] {
    for (;;) {
      auto frame = rsp_reader_->getNextFrame();
      if (frame) {
        OrderResponse rsp;
        rsp.ParseFromString(reinterpret_cast<char*>(frame->getData()), frame->getDataLength());
        OnOrderResponse(rsp);
      }
    }
  });
  md_sub_.Start();

  trade_msg_thread.join();
}

void Strategy::RegisterAlgoOrderEngine(AlgoOrderEngine* engine) {
  engine->SetOrderSender(&sender_);
  engine->SetPosGetter(&pos_getter_);
  engine->Init();
  algo_order_engines_.emplace_back(engine);
}

void Strategy::Subscribe(const std::vector<std::string>& sub_list) {
  if (backtest_mode_) {
    for (const auto& ticker : sub_list) {
      md_sub_.Subscribe<TickData>(ticker, [this](TickData* tick) {
        OnTickMsg(*tick);
        SendNotification(0);
      });
    }
  } else {
    for (const auto& ticker : sub_list) {
      md_sub_.Subscribe<TickData>(ticker, [this](TickData* tick) { OnTickMsg(*tick); });
    }
  }
}

}  // namespace ft
