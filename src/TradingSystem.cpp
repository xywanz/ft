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

TradingSystem::TradingSystem(FrontType front_type) {
  switch (front_type) {
  case FrontType::CTP:
    api_ = new CtpApi(this);
    break;
  default:
    assert(false);
  }
}

TradingSystem::~TradingSystem() {
  if (api_)
    delete api_;
}

bool TradingSystem::login(const LoginParams& params) {
  if (!api_->login(params)) {
    spdlog::error("[Trader] login. Failed to login");
    return false;
  }

  AsyncStatus status;
  status = api_->query_account();
  if (!status.wait())
    return false;
  is_login_ = true;
  spdlog::info("[Trader] login. Login as {}", params.investor_id());

  // query all positions
  status = api_->query_position("", "");
  if (!status.wait()) {
    spdlog::error("[Trader] login. Failed to query positions");
    return false;
  }

  for (auto& ticker : params.subscribed_list())
    md_center_.emplace(ticker, MdManager(ticker));

  return true;
}

void TradingSystem::update_volume(const std::string& ticker,
                           Direction direction,
                           Offset offset,
                           int traded,
                           int pending_changed) {
  bool is_close = is_offset_close(offset);
  if (is_close)
    direction = opp_direction(direction);

  auto key = to_pos_key(ticker, direction);
  std::unique_lock<std::mutex> lock(position_mutex_);
  auto& pos = positions_[key];

  // TODO(kevin): 这里可能出问题，初始化时如果on_trade比on_position
  // 先到达，那么会出现仓位计算不正确的问题
  if (offset == Offset::OPEN) {
    pos.open_pending += pending_changed;
    pos.volume += traded;
  } else if (is_close) {
    pos.close_pending += pending_changed;
    pos.volume -= traded;
  }

  assert(pos.volume >= 0);
  assert(pos.open_pending >= 0);
  assert(pos.close_pending >= 0);
}

void TradingSystem::update_pnl(const std::string& ticker, double last_price) {
  auto contract = ContractTable::get(ticker);
  if (!contract || contract->size <= 0)
    return;

  std::unique_lock<std::mutex> lock(position_mutex_);
  auto iter = positions_.find(to_pos_key(ticker, Direction::BUY));
  if (iter != positions_.end()) {
    auto& pos = iter->second;
    if (pos.volume > 0)
      pos.pnl = pos.volume * contract->size * (last_price - pos.price);
  }

  iter = positions_.find(to_pos_key(ticker, Direction::SELL));
  if (iter != positions_.end()) {
    auto& pos = iter->second;
    if (pos.volume > 0)
      pos.pnl = pos.volume * contract->size * (pos.price - last_price);
  }
}

bool TradingSystem::send_order(const std::string& ticker, int volume,
                        Direction direction, Offset offset,
                        OrderType type, double price) {
  Order order(ticker, direction, offset, volume, type, price);

  order.order_id = api_->send_order(&order);
  if (order.order_id.empty()) {
    spdlog::error("[Trader] send_order. Ticker: {}, Volume: {}, Type: {}, Price: {:.2f}, "
                    "Direction: {}, Offset: {}",
                    ticker, volume, to_string(type), price,
                    to_string(direction), to_string(offset));
    return false;
  }

  order.status = OrderStatus::SUBMITTING;
  std::unique_lock<std::mutex> lock(order_mutex_);
  orders_.emplace(order.order_id, order);
  lock.unlock();
  update_volume(ticker, direction, offset, 0, volume);

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
  std::unique_lock<std::mutex> lock(position_mutex_);
  for (const auto& [key, pos] : positions_) {
    spdlog::info("[Trader] [Position] Ticker: {}, Direction: {}, Price: {:.2f}, "
                 "Holding: {}, Open Pending: {}, Close Pending: {}, PNL: {:.2f}",
                 pos.ticker,
                 to_string(pos.direction),
                 pos.price,
                 pos.volume,
                 pos.open_pending,
                 pos.close_pending,
                 pos.pnl);
  }
}

bool TradingSystem::mount_strategy(const std::string& ticker, Strategy *strategy) {
  strategy->set_ctx(new QuantitativTradingContext(ticker, this));
  std::unique_lock<std::mutex> lock(strategy_mutex_);
  pending_strategies_.emplace_back(ticker, strategy);
  ++pending_strategy_count_;
}


void TradingSystem::on_tick(const MarketData* data) {
  md_center_[data->ticker].on_tick(data);
  update_pnl(data->ticker, data->last_price);

  if (pending_strategy_count_ > 0) {
    std::unique_lock<std::mutex> lock(strategy_mutex_);
    auto tmp = std::move(pending_strategies_);
    pending_strategy_count_ = 0;
    lock.unlock();
    for (auto& [ticker, strategy] : tmp) {
      strategies_[ticker].emplace_back(strategy);
      strategy->on_init(strategy->get_ctx());
    }
  }

  auto iter = strategies_.find(data->ticker);
  if (iter != strategies_.end()) {
    for (auto strategy : iter->second)
      strategy->on_tick(strategy->get_ctx());
  }
}

void TradingSystem::on_position(const Position* position) {
  spdlog::info("[Trader] on_position. Query position success. Ticker: {}, "
               "Direction: {}, Volume: {}, Price: {:.2f}, Frozen: {}",
               position->ticker, to_string(position->direction),
               position->volume, position->price, position->frozen);
  if (position->volume == 0)
    return;

  std::unique_lock<std::mutex> lock(position_mutex_);
  auto& pos = positions_[to_pos_key(position->ticker, position->direction)];
  pos.symbol = position->symbol;
  pos.exchange = position->exchange;
  pos.ticker = to_ticker(pos.symbol, pos.exchange);
  pos.direction = position->direction;
  pos.yd_volume = position->yd_volume;
  pos.volume = position->volume;
  pos.frozen = position->frozen;
  pos.price = position->price;
  pos.pnl = position->pnl;
}

void TradingSystem::on_account(const Account* account) {
  {
    std::unique_lock<std::mutex> lock(account_mutex_);
    account_ = *account;
  }
  spdlog::info("[Trader] on_account. Account ID: {}, Balance: {}, Fronzen: {}",
               account_.account_id, account_.balance, account_.frozen);
}

void TradingSystem::on_order(const Order* rtn_order) {
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
void TradingSystem::on_trade(const Trade* trade) {
  spdlog::debug("[Trader] on_trade. Ticker: {}, Order ID: {}, Trade ID: {}, "
                "Direction: {}, Offset: {}, Price: {:.2f}, Volume: {}",
                trade->ticker, trade->order_id, trade->trade_id,
                to_string(trade->direction), to_string(trade->offset),
                trade->price, trade->volume);

  trade_record_[trade->ticker].emplace_back(*trade);

  Direction d = trade->direction;
  bool is_close = is_offset_close(trade->offset);
  if (is_close)
    d = opp_direction(d);
  auto key = to_pos_key(trade->ticker, d);

  {
    std::unique_lock<std::mutex> lock(position_mutex_);
    auto iter = positions_.find(key);
    if (iter == positions_.end()) {
      lock.unlock();
      if (is_close) {
        spdlog::error("[Trader] on_trade: position to close not found. Ticker: {}, "
                      "Order ID: {}, Trade ID: {}, Direction: {}, Offset: {}, "
                      "Price: {:.2f}, Volume: {}",
                      trade->ticker, trade->order_id, trade->trade_id,
                      to_string(d), to_string(trade->offset),
                      trade->price, trade->volume);
        return;
      }

      Position pos;
      pos.symbol = trade->symbol;
      pos.exchange = trade->exchange;
      pos.ticker = trade->ticker;
      pos.direction = trade->direction;
      pos.volume = trade->volume;
      pos.price = trade->price;
      lock.lock();
      positions_.emplace(std::move(key), std::move(pos));
    }
  }

  update_volume(trade->ticker, trade->direction, trade->offset,
                trade->volume, -trade->volume);

  auto contract = ContractTable::get(trade->ticker);
  if (!contract) {
    spdlog::error("[Trader] on_trade. Contract not found. Ticker: {}", trade->ticker);
    return;
  }

  std::unique_lock<std::mutex> lock(position_mutex_);
  auto& pos = positions_[key];
  double cost = contract->size * (pos.volume - trade->volume) * pos.price;

  if (trade->offset == Offset::OPEN)
    cost += contract->size * trade->volume * trade->price;
  else if (is_offset_close(trade->offset))
    cost -= contract->size * trade->volume * trade->price;

  if (pos.volume > 0 && contract->size > 0)
      pos.price = cost / (pos.volume * contract->size);
}

void TradingSystem::handle_canceled(const Order* rtn_order) {
  {
    std::unique_lock<std::mutex> lock(order_mutex_);
    orders_.erase(rtn_order->order_id);
  }

  auto left_vol = rtn_order->volume - rtn_order->volume_traded;
  update_volume(rtn_order->ticker, rtn_order->direction, rtn_order->offset, 0, -left_vol);
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
