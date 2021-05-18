// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_INCLUDE_FT_STRATEGY_STRATEGY_H_
#define FT_INCLUDE_FT_STRATEGY_STRATEGY_H_

#include <mutex>
#include <string>
#include <vector>

#include "ft/base/config.h"
#include "ft/base/market_data.h"
#include "ft/base/trade_msg.h"
#include "ft/component/yijinjing/journal/JournalReader.h"
#include "ft/component/yijinjing/journal/JournalWriter.h"
#include "ft/strategy/algo_order/algo_order_engine.h"
#include "ft/strategy/order_sender.h"
#include "ft/utils/redis_position_helper.h"
#include "ft/utils/spinlock.h"

namespace ft {

class Strategy {
 public:
  Strategy();

  bool Init(const StrategyConfig& config);

  virtual ~Strategy() {}

  virtual void OnInit() {}

  virtual void OnTick(const TickData& tick) {}

  virtual void OnOrder(const OrderResponse& order) {}

  virtual void OnTrade(const OrderResponse& trade) {}

  virtual void OnExit() {}

  // 仅供加载器调用，内部不可使用
  void Run();

  void RunBacktest();

  // 策略启动后请勿更改id
  void SetStrategyId(const std::string& name) {
    strncpy(strategy_id_, name.c_str(), sizeof(strategy_id_) - 1);
    sender_.SetStrategyId(name);
  }

  void SetAccountId(uint64_t account_id) {
    account_id_ = account_id;
    sender_.SetAccount(account_id);
    pos_getter_.SetAccount(account_id);
  }

  void SetBacktestMode(bool backtest_mode = true) { backtest_mode_ = backtest_mode; }

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
               OrderType type = OrderType::kFak, uint32_t client_order_id = 0) {
    sender_.SendOrder(ticker, volume, Direction::kBuy, Offset::kOpen, type, price, client_order_id);
  }

  void BuyClose(const std::string& ticker, int volume, double price,
                OrderType type = OrderType::kFak, uint32_t client_order_id = 0) {
    sender_.SendOrder(ticker, volume, Direction::kBuy, Offset::kCloseToday, type, price,
                      client_order_id);
  }

  void SellOpen(const std::string& ticker, int volume, double price,
                OrderType type = OrderType::kFak, uint32_t client_order_id = 0) {
    sender_.SendOrder(ticker, volume, Direction::kSell, Offset::kOpen, type, price,
                      client_order_id);
  }

  void SellClose(const std::string& ticker, int volume, double price,
                 OrderType type = OrderType::kFak, uint32_t client_order_id = 0) {
    sender_.SendOrder(ticker, volume, Direction::kSell, Offset::kCloseToday, type, price,
                      client_order_id);
  }

  void CancelOrder(uint64_t order_id) { sender_.CancelOrder(order_id); }

  void CancelForTicker(const std::string& ticker) { sender_.CancelForTicker(ticker); }

  void CancelAll() { sender_.CancelAll(); }

  void SendNotification(uint64_t signal) { sender_.SendNotification(signal); }

  Position GetPosition(const std::string& ticker) const {
    Position pos{};
    pos_getter_.get(ticker, &pos);
    return pos;
  }

  uint64_t GetAccountId() const { return account_id_; }

 private:
  void SendOrder(const std::string& ticker, int volume, Direction direction, Offset offset,
                 OrderType type, double price, uint32_t client_order_id) {
    sender_.SendOrder(ticker, volume, direction, offset, type, price, client_order_id);
  }

 private:
  uint64_t account_id_;
  StrategyIdType strategy_id_;
  OrderSender sender_;
  RedisPositionGetter pos_getter_;
  yijinjing::JournalReaderPtr md_reader_;
  yijinjing::JournalReaderPtr rsp_reader_;
  bool backtest_mode_ = false;

  SpinLock spinlock_;
  std::vector<AlgoOrderEngine*> algo_order_engines_;
};

#define EXPORT_STRATEGY(type) \
  extern "C" ft::Strategy* CreateStrategy() { return new type; }

}  // namespace ft

#endif  // FT_INCLUDE_FT_STRATEGY_STRATEGY_H_
