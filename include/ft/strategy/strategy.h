// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_INCLUDE_FT_STRATEGY_STRATEGY_H_
#define FT_INCLUDE_FT_STRATEGY_STRATEGY_H_

#include <mutex>
#include <string>
#include <vector>

#include "ft/base/config.h"
#include "ft/base/market_data.h"
#include "ft/base/trade_msg.h"
#include "ft/component/trader_db.h"
#include "ft/component/yijinjing/journal/JournalReader.h"
#include "ft/component/yijinjing/journal/JournalWriter.h"
#include "ft/strategy/algo_order/algo_order_engine.h"
#include "ft/strategy/order_sender.h"
#include "ft/utils/spinlock.h"

namespace ft {

class StrategyRunner {
 public:
  virtual ~StrategyRunner() {}

  virtual bool Init(const StrategyConfig& config, const FlareTraderConfig& ft_config) = 0;

  virtual void Run() = 0;

  virtual void RunBacktest() = 0;
};

class Strategy : public StrategyRunner {
 public:
  Strategy();

  bool Init(const StrategyConfig& config, const FlareTraderConfig& ft_config) override;

  void Run() override;

  void RunBacktest() override;

  virtual void OnInit() {}

  virtual void OnTick(const TickData& tick) {}

  virtual void OnOrder(const OrderResponse& order) {}

  virtual void OnTrade(const OrderResponse& trade) {}

  virtual void OnExit() {}

 protected:
  void OnOrderResponse(const OrderResponse& order_rsp) {
    std::unique_lock<SpinLock> lock(spinlock_);

    OnOrder(order_rsp);
    for (auto algo_order_engine : algo_order_engines_) {
      algo_order_engine->OnOrder(order_rsp);
    }

    if (order_rsp.this_traded > 0) {
      OnTrade(order_rsp);
      for (auto algo_order_engine : algo_order_engines_) {
        algo_order_engine->OnTrade(order_rsp);
      }
    }
  }

  void OnTickMsg(const TickData& tick) {
    std::unique_lock<SpinLock> lock(spinlock_);
    for (auto algo_order_engine : algo_order_engines_) {
      algo_order_engine->OnTick(tick);
    }
    OnTick(tick);
  }

  void RegisterAlgoOrderEngine(AlgoOrderEngine* engine);

  void Subscribe(const std::vector<std::string>& sub_list);

  void BuyOpen(const std::string& ticker, int volume, double price,
               OrderType type = OrderType::kFak, uint32_t client_order_id = 0,
               uint64_t timestamp_us = 0) {
    sender_.SendOrder(ticker, volume, Direction::kBuy, Offset::kOpen, type, price, client_order_id,
                      timestamp_us);
  }

  void BuyClose(const std::string& ticker, int volume, double price,
                OrderType type = OrderType::kFak, uint32_t client_order_id = 0,
                uint64_t timestamp_us = 0) {
    sender_.SendOrder(ticker, volume, Direction::kBuy, Offset::kCloseToday, type, price,
                      client_order_id, timestamp_us);
  }

  void SellOpen(const std::string& ticker, int volume, double price,
                OrderType type = OrderType::kFak, uint32_t client_order_id = 0,
                uint64_t timestamp_us = 0) {
    sender_.SendOrder(ticker, volume, Direction::kSell, Offset::kOpen, type, price, client_order_id,
                      timestamp_us);
  }

  void SellClose(const std::string& ticker, int volume, double price,
                 OrderType type = OrderType::kFak, uint32_t client_order_id = 0,
                 uint64_t timestamp_us = 0) {
    sender_.SendOrder(ticker, volume, Direction::kSell, Offset::kCloseToday, type, price,
                      client_order_id, timestamp_us);
  }

  void CancelOrder(uint64_t order_id) { sender_.CancelOrder(order_id); }

  void CancelForTicker(const std::string& ticker) { sender_.CancelForTicker(ticker); }

  void CancelAll() { sender_.CancelAll(); }

  void SendNotification(uint64_t signal) { sender_.SendNotification(signal); }

  Position GetPosition(const std::string& ticker) const {
    Position pos{};
    trader_db_.GetPosition(strategy_id_, ticker, &pos);
    return pos;
  }

  uint64_t GetAccountId() const { return account_id_; }

 private:
  void SendOrder(const std::string& ticker, int volume, Direction direction, Offset offset,
                 OrderType type, double price, uint32_t client_order_id, uint64_t timestamp_us) {
    sender_.SendOrder(ticker, volume, direction, offset, type, price, client_order_id,
                      timestamp_us);
  }

 private:
  uint64_t account_id_;
  StrategyIdType strategy_id_;
  OrderSender sender_;
  TraderDB trader_db_;
  yijinjing::JournalReaderPtr md_reader_;
  yijinjing::JournalReaderPtr rsp_reader_;

  SpinLock spinlock_;
  std::vector<AlgoOrderEngine*> algo_order_engines_;
};

#define EXPORT_STRATEGY(type) \
  extern "C" ft::StrategyRunner* CreateStrategy() { return new type; }

}  // namespace ft

#endif  // FT_INCLUDE_FT_STRATEGY_STRATEGY_H_
