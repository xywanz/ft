// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "ft/strategy/strategy.h"

#include <thread>

#include "ft/component/yijinjing/journal/Timer.h"
#include "spdlog/spdlog.h"

namespace ft {

Strategy::Strategy() {}

bool Strategy::Init(const StrategyConfig& config, const FlareTraderConfig& ft_config) {
  if (!ft::ContractTable::Init(ft_config.global_config.contract_file)) {
    printf("invalid contract list file");
    return false;
  }

  rsp_reader_ = yijinjing::JournalReader::create(".", config.rsp_mq_name, yijinjing::getNanoTime(),
                                                 config.strategy_name);
  md_reader_ = yijinjing::JournalReader::create(".", config.md_mq_name, yijinjing::getNanoTime(),
                                                config.strategy_name);
  sender_.Init(config.trade_mq_name);
  sender_.SetStrategyId(config.strategy_name.c_str());

  account_id_ = std::stoul(ft_config.gateway_config.investor_id);
  pos_getter_.SetAccount(account_id_);

  return true;
}

void Strategy::Run() {
  OnInit();

  for (;;) {
    auto frame = rsp_reader_->getNextFrame();
    if (frame) {
      OrderResponse rsp;
      rsp.ParseFromString(reinterpret_cast<char*>(frame->getData()), frame->getDataLength());
      OnOrderResponse(rsp);
    }

    frame = md_reader_->getNextFrame();
    if (frame) {
      TickData tick;
      tick.ParseFromString(reinterpret_cast<char*>(frame->getData()), frame->getDataLength());
      OnTick(tick);
    }
  }
}

void Strategy::RunBacktest() {
  OnInit();
  SendNotification(0);

  for (;;) {
    auto frame = rsp_reader_->getNextFrame();
    if (frame) {
      OrderResponse rsp;
      rsp.ParseFromString(reinterpret_cast<char*>(frame->getData()), frame->getDataLength());
      OnOrderResponse(rsp);
    }

    frame = md_reader_->getNextFrame();
    if (frame) {
      TickData tick;
      tick.ParseFromString(reinterpret_cast<char*>(frame->getData()), frame->getDataLength());
      OnTick(tick);
      SendNotification(0);
    }
  }
}

void Strategy::RegisterAlgoOrderEngine(AlgoOrderEngine* engine) {
  engine->SetOrderSender(&sender_);
  engine->SetPosGetter(&pos_getter_);
  engine->Init();
  algo_order_engines_.emplace_back(engine);
}

void Strategy::Subscribe(const std::vector<std::string>& sub_list) {}

}  // namespace ft
