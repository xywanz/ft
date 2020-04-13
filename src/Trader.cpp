// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "Trader.h"

#include <cassert>
#include <iostream>
#include <set>
#include <vector>

#include <spdlog/spdlog.h>

#include "ctp/CtpGateway.h"
#include "ctp/CtpMdReceiver.h"
#include "LoginParams.h"

namespace ft {

Trader::Trader(FrontType front_type) {
  switch (front_type) {
  case FrontType::CTP:
    gateway_ = new CtpGateway;
    md_receiver_ = new CtpMdReceiver;
    break;
  default:
    assert(false);
  }

  gateway_->register_cb(this);
  md_receiver_->register_cb(this);
}

Trader::~Trader() {
  if (gateway_)
    delete gateway_;
}

bool Trader::login(const LoginParams& params) {
  if (!md_receiver_->login(params)) {
    spdlog::error("[Trader] login. Failed to login into MD server");
    return false;
  }

  if (!gateway_->login(params)) {
    spdlog::error("[Trader] login. Failed to login into trading server");
    return false;
  }

  AsyncStatus status;
  status = gateway_->query_account();
  if (!status.wait())
    return false;
  is_login_ = true;
  spdlog::info("[Trader] login. Login as {}", params.investor_id());

  // query all positions
  status = gateway_->query_position("", "");
  if (!status.wait()) {
    spdlog::error("[Trader] login. Failed to query positions");
    return false;
  }

  for (auto& ticker : params.subscribed_list())
    md_center_.emplace(ticker, MdManager(ticker));

  return true;
}

void Trader::update_pending(const std::string& ticker,
                            Direction direction,
                            Offset offset,
                            int volume) {
  bool is_close = is_offset_close(offset);
  if (is_close)
    direction = opp_direction(direction);

  auto key = to_pos_key(ticker, direction);
  std::unique_lock<std::mutex> lock(position_mutex_);
  auto& pos = positions_[key];
  if (pos.ticker.empty()) {
    pos.ticker = ticker;
    pos.direction = direction;
  }

  if (offset == Offset::OPEN)
    pos.open_pending += volume;
  else if (is_close)
    pos.close_pending += volume;

  assert(pos.open_pending >= 0);
  assert(pos.close_pending >= 0);
}

void Trader::update_traded(const std::string& ticker,
                           Direction direction,
                           Offset offset,
                           int volume) {
  bool is_close = is_offset_close(offset);
  if (is_close)
    direction = opp_direction(direction);

  auto key = to_pos_key(ticker, direction);
  std::unique_lock<std::mutex> lock(position_mutex_);
  auto& pos = positions_[key];

  // TODO(kevin): 这里可能出问题，初始化时如果on_trade比on_position
  // 先到达，那么会出现仓位计算不正确的问题
  if (offset == Offset::OPEN) {
    pos.open_pending -= volume;
    pos.volume += volume;
  } else if (is_close) {
    pos.close_pending -= volume;
    pos.volume -= volume;
  }

  assert(pos.volume >= 0);
  assert(pos.open_pending >= 0);
  assert(pos.close_pending >= 0);
}

void Trader::update_pnl(const std::string& ticker, double last_price) {
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

bool Trader::send_order(const std::string& ticker, int volume,
                        Direction direction, Offset offset,
                        OrderType type, double price) {
  Order order(ticker, direction, offset, volume, type, price);

  order.order_id = gateway_->send_order(&order);
  if (order.order_id.empty()) {
    spdlog::error("[Trader] send_order. Ticker: {}, Volume: {}, Type: {}, Price: {}, "
                    "Direction: {}, Offset: {}",
                    ticker, volume, to_string(type), price,
                    to_string(direction), to_string(offset));
    return false;
  }

  order.status = OrderStatus::SUBMITTING;
  std::unique_lock<std::mutex> lock(order_mutex_);
  orders_.emplace(order.order_id, order);
  lock.unlock();
  update_pending(ticker, direction, offset, volume);

  spdlog::debug("[Trader] send_order. Ticker: {}, Volume: {}, Type: {}, Price: {}, "
                "Direction: {}, Offset: {}",
                ticker, volume, to_string(type), price,
                to_string(direction), to_string(offset));

  return true;
}

bool Trader::cancel_order(const std::string& order_id) {
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

  if (!gateway_->cancel_order(order_id)) {
    order.flags.reset(kCancelBit);
    spdlog::error("[Trader] cancel_order. Failed: unknown error");
    return false;
  }

  spdlog::debug("[Trader] cancel_order. OrderID: {}, Ticker: {}, LeftVolume: {}",
                order_id, order.ticker, order.volume - order.volume_traded);

  return true;
}

void Trader::get_orders(std::vector<const Order*>* out) {
  for (const auto& [order_id, order] : orders_)
    out->push_back(&order);
}

void Trader::get_orders(const std::string&ticker, std::vector<const Order*>* out) {
  for (const auto& [order_id, order] : orders_) {
    if (order.ticker == ticker)
      out->push_back(&order);
  }
}

void Trader::show_positions() {
  std::unique_lock<std::mutex> lock(position_mutex_);
  for (const auto& [key, pos] : positions_) {
    spdlog::info("[Trader] [Position] Ticker: {}, Direction: {}, Price: {:.2f}, "
                 "Holding: {}, Open Pending: {}, Close Pending: {}, PNL: {}",
                 pos.ticker,
                 to_string(pos.direction),
                 pos.price,
                 pos.volume,
                 pos.open_pending,
                 pos.close_pending,
                 pos.pnl);
  }
}


void Trader::on_market_data(const MarketData* data) {
  md_center_[data->ticker].on_tick(data);
  update_pnl(data->ticker, data->last_price);
}

void Trader::on_position(const Position* position) {
  spdlog::info("[Trader] on_position. Query position success. Ticker: {}, "
               "Direction: {}, Volume: {}, Price: {}, Frozen: {}",
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

void Trader::on_account(const Account* account) {
  {
    std::unique_lock<std::mutex> lock(account_mutex_);
    account_ = *account;
  }
  spdlog::info("[Trader] on_account. Account ID: {}, Balance: {}, Fronzen: {}",
               account_.account_id, account_.balance, account_.frozen);
}

void Trader::on_order(const Order* rtn_order) {
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
void Trader::on_trade(const Trade* trade) {
  spdlog::debug("[Trader] on_trade. Ticker: {}, Order ID: {}, Trade ID: {}, "
                "Direction: {}, Offset: {}, Price: {}, Volume: {}",
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
                      "Price: {}, Volume: {}",
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

  update_traded(trade->ticker, trade->direction, trade->offset, trade->volume);

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

void Trader::handle_canceled(const Order* rtn_order) {
  {
    std::unique_lock<std::mutex> lock(order_mutex_);
    orders_.erase(rtn_order->order_id);
  }

  auto left_vol = rtn_order->volume - rtn_order->volume_traded;
  update_pending(rtn_order->ticker, rtn_order->direction, rtn_order->offset, -left_vol);
}

void Trader::handle_submitted(const Order* rtn_order) {
  std::unique_lock<std::mutex> lock(order_mutex_);
  auto& order = orders_[rtn_order->order_id];
  order.status = OrderStatus::NO_TRADED;
}

void Trader::handle_part_traded(const Order* rtn_order) {
  std::unique_lock<std::mutex> lock(order_mutex_);
  auto& order = orders_[rtn_order->order_id];
  if (rtn_order->volume_traded > order.volume_traded)
    order.volume_traded = rtn_order->volume_traded;
  order.status = OrderStatus::PART_TRADED;
}

void Trader::handle_all_traded(const Order* rtn_order) {
  std::unique_lock<std::mutex> lock(order_mutex_);
  orders_.erase(rtn_order->order_id);
}

void Trader::handle_cancel_rejected(const Order* rtn_order) {
  int volume;

  {
    std::unique_lock<std::mutex> lock(order_mutex_);
    auto& order = orders_[rtn_order->order_id];
    volume = order.volume - order.volume_traded;
    order.flags.reset(kCancelBit);
  }

  update_pending(rtn_order->ticker, rtn_order->direction, rtn_order->offset, -volume);
}

}  // namespace ft
