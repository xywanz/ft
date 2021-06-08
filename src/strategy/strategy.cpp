// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "ft/strategy/strategy.h"

#include <thread>

#include "ft/component/yijinjing/journal/Timer.h"
#include "spdlog/spdlog.h"

namespace ft {

Strategy::Strategy() {}

bool Strategy::Init(const StrategyConfig& config, const FlareTraderConfig& ft_config) {
  if (!ft::ContractTable::Init(ft_config.global_config.contract_file)) {
    printf("invalid contract list file\n");
    return false;
  }

  if (!trader_db_.Init(ft_config.global_config.trader_db_address, "", "")) {
    printf("cannot open db connection\n");
    return false;
  }

  rsp_reader_ = yijinjing::JournalReader::create(".", config.rsp_mq_name, yijinjing::getNanoTime(),
                                                 config.strategy_name);
  md_reader_ = yijinjing::JournalReader::create(".", config.md_mq_name, yijinjing::getNanoTime(),
                                                config.strategy_name);
  sender_.Init(config.trade_mq_name);
  sender_.SetStrategyId(config.strategy_name.c_str());

  account_id_ = std::stoul(ft_config.gateway_config.investor_id);
  strncpy(strategy_id_, config.strategy_name.c_str(), sizeof(strategy_id_));

  return true;
}

void Strategy::Run() {
  OnInit();

  for (;;) {
    auto frame = rsp_reader_->getNextFrame();
    if (frame) {
      if (frame->getDataLength() != sizeof(OrderResponse)) {
        printf("invalid order rsp len\n");
        abort();
      }
      auto* rsp = reinterpret_cast<OrderResponse*>(frame->getData());
      OnOrderResponse(*rsp);
    }

    frame = md_reader_->getNextFrame();
    if (frame) {
      if (frame->getDataLength() != sizeof(TickData)) {
        printf("invalid tick data len\n");
        abort();
      }
      TickData* tick = reinterpret_cast<TickData*>(frame->getData());
      OnTick(*tick);
    }
  }
}

void Strategy::RunBacktest() {
  OnInit();
  SendNotification(0);

  for (;;) {
    auto frame = rsp_reader_->getNextFrame();
    if (frame) {
      if (frame->getDataLength() != sizeof(OrderResponse)) {
        printf("invalid order rsp len\n");
        abort();
      }
      auto* rsp = reinterpret_cast<OrderResponse*>(frame->getData());
      OnOrderResponse(*rsp);
    }

    frame = md_reader_->getNextFrame();
    if (frame) {
      if (frame->getDataLength() != sizeof(TickData)) {
        printf("invalid tick data len\n");
        abort();
      }
      TickData* tick = reinterpret_cast<TickData*>(frame->getData());
      OnTick(*tick);
      SendNotification(0);
    }
  }
}

void Strategy::RegisterAlgoOrderEngine(AlgoOrderEngine* engine) {
  engine->SetStrategyName(strategy_id_);
  engine->SetOrderSender(&sender_);
  engine->SetTraderDB(&trader_db_);
  engine->Init();
  algo_order_engines_.emplace_back(engine);
}

void Strategy::Subscribe(const std::vector<std::string>& sub_list) {}

}  // namespace ft
