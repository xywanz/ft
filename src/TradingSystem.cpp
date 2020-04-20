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

  auto& pos = positions_[ticker];
  if (pos.ticker.empty()) {
    pos.ticker = ticker;
    ticker_split(pos.ticker, &pos.symbol, &pos.exchange);
  }
  auto& pos_detail = direction == Direction::BUY ? pos.long_pos : pos.short_pos;

  // TODO(kevin): 这里可能出问题，初始化时如果on_trade比on_position
  // 先到达，那么会出现仓位计算不正确的问题
  if (offset == Offset::OPEN) {
    pos_detail.open_pending += pending_changed;
    pos_detail.volume += traded;
  } else if (is_close) {
    pos_detail.close_pending += pending_changed;
    pos_detail.volume -= traded;
  }

  assert(pos_detail.volume >= 0);
  assert(pos_detail.open_pending >= 0);
  assert(pos_detail.close_pending >= 0);
}

void TradingSystem::update_pnl(const std::string& ticker, double last_price) {
  auto contract = ContractTable::get_by_ticker(ticker);
  if (!contract || contract->size <= 0)
    return;

  auto iter = positions_.find(ticker);
  if (iter == positions_.end())
    return;
  auto& pos = iter->second;

  auto& lp = pos.long_pos;
  if (lp.volume > 0)
    lp.pnl = lp.volume * contract->size * (last_price - lp.cost_price);

  auto& sp = pos.short_pos;
  if (sp.volume > 0)
    sp.pnl = sp.volume * contract->size * (sp.cost_price - last_price);
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
  for (const auto& [ticker, pos] : positions_) {
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

  positions_.emplace(position->ticker, *position);
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

  auto iter = positions_.find(trade->ticker);
  if (iter == positions_.end()) {
    if (is_close) {
      spdlog::error("[Trader] on_trade: position to close not found. Ticker: {}, "
                    "Order ID: {}, Trade ID: {}, Direction: {}, Offset: {}, "
                    "Price: {:.2f}, Volume: {}",
                    trade->ticker, trade->order_id, trade->trade_id,
                    to_string(d), to_string(trade->offset),
                    trade->price, trade->volume);
    } else {
      Position pos(trade->symbol, trade->exchange);
      auto& pos_detail = d == Direction::BUY ? pos.long_pos : pos.short_pos;
      pos_detail.volume = trade->volume;
      pos_detail.cost_price = trade->price;
      positions_.emplace(pos.ticker, pos);

      spdlog::warn("[Trader] on_trade arrived early than on_position");
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

  auto& pos = iter->second;
  auto& pos_detail = d == Direction::BUY ? pos.long_pos : pos.short_pos;
  double cost = contract->size * (pos_detail.volume - trade->volume) * pos_detail.cost_price;

  if (trade->offset == Offset::OPEN)
    cost += contract->size * trade->volume * trade->price;
  else if (is_offset_close(trade->offset))
    cost -= contract->size * trade->volume * trade->price;

  if (pos_detail.volume > 0 && contract->size > 0)
      pos_detail.cost_price = cost / (pos_detail.volume * contract->size);
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
