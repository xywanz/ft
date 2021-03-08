// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "ft/strategy/strategy.h"

#include <thread>

#include "spdlog/spdlog.h"

namespace ft {

Strategy::Strategy()
    : md_sub_("ipc://md.ft_trader.ipc"), trade_msg_sub_("ipc://trade_msg.ft_trader.ipc") {
  std::this_thread::sleep_for(std::chrono::milliseconds(200));  // 等待zmq连接建立
}

void Strategy::Run() {
  trade_msg_sub_.Subscribe<OrderResponse>(strategy_id_,
                                          [this](OrderResponse* rsp) { OnOrderResponse(*rsp); });

  OnInit();
  if (backtest_mode_) {
    SendNotification(0);
  }

  std::thread trade_msg_thread([this] { trade_msg_sub_.Start(); });
  md_sub_.Start();

  trade_msg_thread.join();
}

void Strategy::RegisterAlgoOrderEngine(AlgoOrderEngine* engine) {
  engine->SetOrderSender(&sender_);
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
