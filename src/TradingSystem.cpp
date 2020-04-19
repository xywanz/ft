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
  engine_->set_handler(EV_SHOW_POSITION, MEM_HANDLER(TradingSystem::on_show_position));

  engine_->run(false);
}

TradingSystem::~TradingSystem() {
  close();
}

bool TradingSystem::login(const LoginParams& params) {
  if (!api_->login(params)) {
    spdlog::error("[Trader] login. Failed to login");
    return false;
  }

  if (!api_->query_account())
    return false;
  is_login_ = true;
  spdlog::info("[Trader] login. Login as {}", params.investor_id());

  // query all positions
  if (!api_->query_position("", "")) {
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

  auto& pos = positions_[to_pos_key(ticker, direction)];
  if (pos.ticker.empty()) {
    pos.ticker = ticker;
    ticker_split(pos.ticker, &pos.symbol, &pos.exchange);
    pos.direction = direction;
  }

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
  auto contract = ContractTable::get_by_ticker(ticker);
  if (!contract || contract->size <= 0)
    return;

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
  order.status = OrderStatus::SUBMITTING;

  order.order_id = api_->send_order(&order);
  if (order.order_id.empty()) {
    spdlog::error("[Trader] send_order. Ticker: {}, Volume: {}, Type: {}, Price: {:.2f}, "
                    "Direction: {}, Offset: {}",
                    ticker, volume, to_string(type), price,
                    to_string(direction), to_string(offset));
    return false;
  }

  {
    std::unique_lock<std::mutex> lock(order_mutex_);
    orders_.emplace(order.order_id, order);
  }

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
  engine_->post(EV_SHOW_POSITION);
}

void TradingSystem::on_show_position(cppex::Any*) {
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
  auto strategy_unique_ptr = data->fetch<Strategy>();
  auto* strategy = strategy_unique_ptr.release();

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
  auto* tick = data->cast<MarketData>();
  update_pnl(tick->ticker, tick->last_price);
  md_center_[tick->ticker].on_tick(tick);
  ticks_[tick->ticker].emplace_back(*tick);

  auto iter = strategies_.find(tick->ticker);
  if (iter != strategies_.end()) {
    auto& strategy_list = iter->second;
    for (auto& strategy : strategy_list)
      strategy->on_tick(strategy->get_ctx());
  }
}

void TradingSystem::on_position(cppex::Any* data) {
  auto* position = data->cast<Position>();
  spdlog::info("[Trader] on_position. Query position success. Ticker: {}, "
               "Direction: {}, Volume: {}, Price: {:.2f}, Frozen: {}",
               position->ticker, to_string(position->direction),
               position->volume, position->price, position->frozen);
  if (position->volume == 0)
    return;

  auto& pos = positions_[to_pos_key(position->ticker, position->direction)];
  pos.symbol = position->symbol;
  pos.exchange = position->exchange;
  pos.ticker = position->ticker;
  pos.direction = position->direction;
  pos.yd_volume = position->yd_volume;
  pos.volume = position->volume;
  pos.frozen = position->frozen;
  pos.price = position->price;
  pos.pnl = position->pnl;
}

void TradingSystem::on_account(cppex::Any* data) {
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

  auto d = trade->direction;
  bool is_close = is_offset_close(trade->offset);
  if (is_close)
    d = opp_direction(d);
  auto key = to_pos_key(trade->ticker, d);

  auto iter = positions_.find(key);
  if (iter == positions_.end()) {
    if (is_close) {
      spdlog::error("[Trader] on_trade: position to close not found. Ticker: {}, "
                    "Order ID: {}, Trade ID: {}, Direction: {}, Offset: {}, "
                    "Price: {:.2f}, Volume: {}",
                    trade->ticker, trade->order_id, trade->trade_id,
                    to_string(d), to_string(trade->offset),
                    trade->price, trade->volume);
    } else {
      Position pos;
      pos.symbol = trade->symbol;
      pos.exchange = trade->exchange;
      pos.ticker = trade->ticker;
      pos.direction = trade->direction;
      pos.volume = trade->volume;
      pos.price = trade->price;
      positions_.emplace(std::move(key), std::move(pos));

      spdlog::warn("[Trader] trade arrived early than position");
    }

    return;
  }

  update_volume(trade->ticker, trade->direction, trade->offset,
                trade->volume, -trade->volume);

  auto contract = ContractTable::get_by_ticker(trade->ticker);
  if (!contract) {
    spdlog::error("[Trader] on_trade. Contract not found. Ticker: {}", trade->ticker);
    return;
  }

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
