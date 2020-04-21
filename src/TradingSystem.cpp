// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "TradingSystem.h"

#include <cassert>
#include <iostream>
#include <set>
#include <Strategy.h>
#include <vector>

#include <spdlog/spdlog.h>

#include "ctp/CtpApi.h"
#include "LoginParams.h"

namespace ft {

TradingSystem::TradingSystem(FrontType front_type)
  : engine_(new EventEngine) {
  switch (front_type) {
  case FrontType::CTP:
    api_.reset(new CtpApi(engine_.get()));
    break;
  default:
    assert(false);
  }

  engine_->set_handler(EV_ACCOUNT, MEM_HANDLER(TradingSystem::on_account));
  engine_->set_handler(EV_POSITION, MEM_HANDLER(TradingSystem::on_position));
  engine_->set_handler(EV_ORDER, MEM_HANDLER(TradingSystem::on_order));
  engine_->set_handler(EV_TRADE, MEM_HANDLER(TradingSystem::on_trade));
  engine_->set_handler(EV_TICK, MEM_HANDLER(TradingSystem::on_tick));
  engine_->set_handler(EV_MOUNT_STRATEGY, MEM_HANDLER(TradingSystem::on_mount_strategy));
  engine_->set_handler(EV_UMOUNT_STRATEGY, MEM_HANDLER(TradingSystem::on_unmount_strategy));

  engine_->run(false);
}

TradingSystem::~TradingSystem() {
  close();
}

void TradingSystem::close() {
    api_.reset();
    if (engine_)
      engine_->stop();
}

bool TradingSystem::login(const LoginParams& params) {
  if (!api_->login(params)) {
    spdlog::error("[Trader] login. Failed to login");
    return false;
  }

  is_login_ = true;
  spdlog::info("[Trader] login. Login as {}", params.investor_id());

  if (!api_->query_account())
    return false;

  // query all positions
  initial_positions_.clear();
  pos_mgr_.clear();
  if (!api_->query_position("", "")) {
    spdlog::error("[Trader] login. Failed to query positions");
    return false;
  }
  for (const auto& pos : initial_positions_)
    pos_mgr_.init_position(pos);
  initial_positions_.clear();

  for (auto& ticker : params.subscribed_list())
    md_center_.emplace(ticker, MdManager(ticker));

  return true;
}

bool TradingSystem::send_order(const std::string& ticker, int volume,
                        Direction direction, Offset offset,
                        OrderType type, double price) {
  Order order(ticker, direction, offset, volume, type, price);
  order.status = OrderStatus::SUBMITTING;

  {
    // 这里加锁是怕成交回执或订单回执在order插入到map前就到了且被处理了
    std::unique_lock<std::mutex> lock(order_mutex_);
    order.order_id = api_->send_order(&order);
    if (order.order_id.empty()) {
      spdlog::error("[Trader] send_order. Ticker: {}, Volume: {}, Type: {}, Price: {:.2f}, "
                    "Direction: {}, Offset: {}",
                    ticker, volume, to_string(type), price,
                    to_string(direction), to_string(offset));
      return false;
    }

    orders_.emplace(order.order_id, order);
  }

  pos_mgr_.update_pending(ticker, direction, offset, volume);

  spdlog::debug("[Trader] send_order. Ticker: {}, Volume: {}, Type: {}, Price: {:.2f}, "
                "Direction: {}, Offset: {}",
                ticker, volume, to_string(type), price,
                to_string(direction), to_string(offset));

  return true;
}

bool TradingSystem::cancel_order(const std::string& order_id) {
  std::unique_lock<std::mutex> lock(order_mutex_);
  auto iter = orders_.find(order_id);
  if (iter == orders_.end()) {
    spdlog::error("[Trader] CancelOrder failed: order not found");
    return false;
  }

  auto& order = iter->second;
  if (order.flags.test(kCancelBit))
    return true;

  order.flags.set(kCancelBit);
  if (!api_->cancel_order(order_id)) {
    order.flags.reset(kCancelBit);
    spdlog::error("[Trader] cancel_order. Failed: unknown error");
    return false;
  }

  spdlog::debug("[Trader] cancel_order. OrderID: {}, Ticker: {}, LeftVolume: {}",
                order_id, order.ticker, order.volume - order.volume_traded);
  return true;
}

void TradingSystem::show_positions() {
  std::vector<std::string> pos_ticker_list;
  pos_mgr_.get_pos_ticker_list(&pos_ticker_list);
  for (const auto& ticker : pos_ticker_list) {
    const auto pos = pos_mgr_.get_position(ticker);
    if (pos.ticker.empty())
      continue;
    auto& lp = pos.long_pos;
    auto& sp = pos.short_pos;
    spdlog::info("[Trader] [Position] Ticker: {}, "
                 "LP: {}, LOP: {}, LCP: {}, LongPrice: {:.2f}, LongPNL: {:.2f}, "
                 "SP: {}, SOP: {}, SCP: {}, ShortPrice: {:.2f}, ShortPNL: {:.2f}",
                 pos.ticker,
                 lp.volume, lp.close_pending, lp.cost_price, lp.pnl, sp.volume,
                 sp.open_pending, sp.close_pending, sp.cost_price, sp.pnl);
  }
}

void TradingSystem::mount_strategy(const std::string& ticker,
                                   Strategy* strategy) {
  strategy->set_ctx(new QuantitativeTradingContext(ticker, this));
  engine_->post(EV_MOUNT_STRATEGY, strategy);
}

void TradingSystem::on_mount_strategy(cppex::Any* data) {
  auto strategy_unique_ptr = data->fetch<Strategy>();
  auto* strategy = strategy_unique_ptr.release();
  auto& list = strategies_[strategy->get_ctx()->this_ticker()];
  list.emplace_back(std::move(strategy));

  strategy->on_init(strategy->get_ctx());
}

void TradingSystem::unmount_strategy(Strategy* strategy) {
  engine_->post(EV_UMOUNT_STRATEGY, strategy);
}

void TradingSystem::on_unmount_strategy(cppex::Any* data) {
  auto* strategy = data->fetch<Strategy>().release();

  auto ctx = strategy->get_ctx();
  if (!ctx)
    return;

  auto iter = strategies_.find(ctx->this_ticker());
  if (iter == strategies_.end())
    return;

  auto& list = iter->second;
  for (auto iter = list.begin(); iter != list.end(); ++iter) {
    if (*iter == strategy) {
      strategy->on_exit(ctx);
      strategy->set_ctx(nullptr);
      list.erase(iter);
      return;
    }
  }
}

void TradingSystem::on_tick(cppex::Any* data) {
  auto* tick = data->fetch<MarketData>().release();

  std::unique_lock<std::mutex> lock(tick_mutex_);
  md_center_[tick->ticker].on_tick(tick);
  ticks_[tick->ticker].emplace_back(tick);
  lock.unlock();

  pos_mgr_.update_pnl(tick->ticker, tick->last_price);

  auto iter = strategies_.find(tick->ticker);
  if (iter != strategies_.end()) {
    auto& strategy_list = iter->second;
    for (auto& strategy : strategy_list)
      strategy->on_tick(strategy->get_ctx());
  }
}

void TradingSystem::on_position(cppex::Any* data) {
  auto position = data->cast<Position>();
  auto& lp = position->long_pos;
  auto& sp = position->short_pos;
  spdlog::info("[Trader] on_position. Query position success. Ticker: {}, "
               "Long Volume: {}, Long Price: {:.2f}, Long Frozen: {}, Long PNL: {}, "
               "Short Volume: {}, Short Price: {:.2f}, Short Frozen: {}, Short PNL: {}",
               position->ticker,
               lp.volume, lp.cost_price, lp.frozen, lp.pnl,
               sp.volume, sp.cost_price, sp.frozen, sp.pnl);

  if (lp.volume == 0 && sp.volume == 0)
    return;

  initial_positions_.emplace_back(*position);
}

void TradingSystem::on_account(cppex::Any* data) {
  std::unique_lock<std::mutex> lock(account_mutex_);
  auto* account = data->cast<Account>();
  account_ = *account;
  spdlog::info("[Trader] on_account. Account ID: {}, Balance: {}, Fronzen: {}",
               account_.account_id, account_.balance, account_.frozen);
}

void TradingSystem::on_order(cppex::Any* data) {
  auto* rtn_order = data->cast<Order>();
  {
    std::unique_lock<std::mutex> lock(order_mutex_);
    if (orders_.find(rtn_order->order_id) == orders_.end()) {
      lock.unlock();
      spdlog::error("[Trader] on_order. Order not found. Ticker: {}, Order ID: {}, "
                    "Direction: {}, Offset: {}, Volume: {}",
                    rtn_order->ticker, rtn_order->order_id,
                    to_string(rtn_order->direction), to_string(rtn_order->offset),
                    rtn_order->volume);
      return;
    }
  }

  switch (rtn_order->status) {
  case OrderStatus::SUBMITTING:
    break;
  case OrderStatus::REJECTED:
  case OrderStatus::CANCELED:
    handle_canceled(rtn_order);
    break;
  case OrderStatus::NO_TRADED:
    handle_submitted(rtn_order);
    break;
  case OrderStatus::PART_TRADED:
    handle_part_traded(rtn_order);
    break;
  case OrderStatus::ALL_TRADED:
    handle_all_traded(rtn_order);
    break;
  case OrderStatus::CANCEL_REJECTED:
    handle_cancel_rejected(rtn_order);
    break;
  default:
    assert(false);
  }

  spdlog::info("[Trader] on_order. Ticker: {}, Order ID: {}, Direction: {}, "
               "Offset: {}, Traded: {}, Origin Volume: {}, Status: {}",
               rtn_order->ticker, rtn_order->order_id, to_string(rtn_order->direction),
               to_string(rtn_order->offset), rtn_order->volume_traded, rtn_order->volume,
               to_string(rtn_order->status));
}

// TODO(Kevin): fix incorrect calculation and missing data
void TradingSystem::on_trade(cppex::Any* data) {
  auto* trade = data->cast<Trade>();
  spdlog::debug("[Trader] on_trade. Ticker: {}, Order ID: {}, Trade ID: {}, "
                "Direction: {}, Offset: {}, Price: {:.2f}, Volume: {}",
                trade->ticker, trade->order_id, trade->trade_id,
                to_string(trade->direction), to_string(trade->offset),
                trade->price, trade->volume);

  trade_record_[trade->ticker].emplace_back(*trade);
  pos_mgr_.update_traded(trade->ticker, trade->direction, trade->offset,
                         trade->volume, trade->price);
}

void TradingSystem::handle_canceled(const Order* rtn_order) {
  {
    std::unique_lock<std::mutex> lock(order_mutex_);
    orders_.erase(rtn_order->order_id);
  }

  auto left_vol = rtn_order->volume - rtn_order->volume_traded;
  pos_mgr_.update_pending(rtn_order->ticker, rtn_order->direction, rtn_order->offset, -left_vol);
}

void TradingSystem::handle_submitted(const Order* rtn_order) {
  std::unique_lock<std::mutex> lock(order_mutex_);
  auto& order = orders_[rtn_order->order_id];
  order.status = OrderStatus::NO_TRADED;
}

void TradingSystem::handle_part_traded(const Order* rtn_order) {
  std::unique_lock<std::mutex> lock(order_mutex_);
  auto& order = orders_[rtn_order->order_id];
  if (rtn_order->volume_traded > order.volume_traded)
    order.volume_traded = rtn_order->volume_traded;
  order.status = OrderStatus::PART_TRADED;
}

void TradingSystem::handle_all_traded(const Order* rtn_order) {
  std::unique_lock<std::mutex> lock(order_mutex_);
  orders_.erase(rtn_order->order_id);
}

void TradingSystem::handle_cancel_rejected(const Order* rtn_order) {
    std::unique_lock<std::mutex> lock(order_mutex_);
    auto& order = orders_[rtn_order->order_id];
    order.flags.reset(kCancelBit);
}

}  // namespace ft
