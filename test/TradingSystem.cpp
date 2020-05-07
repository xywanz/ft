// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "TradingSystem.h"

#include <spdlog/spdlog.h>

#include <cassert>
#include <set>
#include <vector>

#include "Base/DataStruct.h"
#include "RiskManagement/NoSelfTrade.h"

namespace ft {

TradingSystem::TradingSystem() : engine_(new EventEngine) {
  engine_->set_handler(EV_ACCOUNT, MEM_HANDLER(TradingSystem::on_account));
  engine_->set_handler(EV_POSITION, MEM_HANDLER(TradingSystem::on_position));
  engine_->set_handler(EV_ORDER, MEM_HANDLER(TradingSystem::on_order));
  engine_->set_handler(EV_TRADE, MEM_HANDLER(TradingSystem::on_trade));
  engine_->set_handler(EV_TICK, MEM_HANDLER(TradingSystem::on_tick));
  engine_->run(false);
}

TradingSystem::~TradingSystem() {}

void TradingSystem::close() {
  api_->logout();
  engine_->stop();
}

bool TradingSystem::login(const LoginParams& params) {
  api_.reset(create_gateway(params.api(), engine_.get()));
  if (!api_) {
    spdlog::error("[TradingSystem::login] Unknown api");
    return false;
  }

  if (!api_->login(params)) {
    spdlog::error("[TradingSystem::login] Failed to login");
    return false;
  }

  spdlog::info("[TradingSystem] login. Login as {}", params.investor_id());

  if (!api_->query_account()) {
    spdlog::error("[TradingSystem::login] Failed to query account");
    return false;
  }

  // query all positions
  is_process_pos_done_ = false;
  if (!api_->query_positions()) {
    spdlog::error("[TradingSystem::login] Failed to query positions");
    return false;
  }

  engine_->post(EV_SYNC);
  while (!is_process_pos_done_) continue;

  for (auto& ticker : params.subscribed_list())
    tick_center_.emplace(ticker, TickDB(ticker));

  return true;
}

std::string TradingSystem::send_order(const std::string& ticker, int volume,
                                      Direction direction, Offset offset,
                                      OrderType type, double price) {
  std::unique_lock<std::mutex> lock(mutex_);
  Order order(ticker, direction, offset, volume, type, price);
  order.status = OrderStatus::SUBMITTING;

  if (!risk_mgr_.check(&order)) {
    spdlog::error("[TradingSystem::send_order] RiskMgr check failed");
    return "";
  }

  order.order_id = api_->send_order(&order);
  if (order.order_id.empty()) {
    spdlog::error(
        "[TradingSystem] send_order. Ticker: {}, Volume: {}, Type: {}, Price: "
        "{:.2f}, "
        "Direction: {}, Offset: {}",
        ticker, volume, to_string(type), price, to_string(direction),
        to_string(offset));
    return "";
  }

  panel_.process_new_order(&order);
  panel_.update_pos_pending(ticker, direction, offset, volume);

  spdlog::debug(
      "[TradingSystem] send_order. Ticker: {}, Volume: {}, Type: {}, Price: "
      "{:.2f}, "
      "Direction: {}, Offset: {}",
      ticker, volume, to_string(type), price, to_string(direction),
      to_string(offset));

  return order.order_id;
}

bool TradingSystem::cancel_order(const std::string& order_id) {
  std::unique_lock<std::mutex> lock(mutex_);
  const Order* order = panel_.get_order_by_id(order_id);
  if (!order) {
    spdlog::error("[TradingSystem] CancelOrder failed: order not found");
    return false;
  }

  if (!api_->cancel_order(order_id)) {
    spdlog::error("[TradingSystem] cancel_order. Failed: unknown error");
    return false;
  }

  spdlog::debug(
      "[TradingSystem] cancel_order. OrderID: {}, Ticker: {}, LeftVolume: {}",
      order_id, order->ticker, order->volume - order->volume_traded);
  return true;
}

// void TradingSystem::show_positions() {
//   std::vector<std::string> pos_ticker_list;
//   portfolio_.get_pos_ticker_list(&pos_ticker_list);
//   for (const auto& ticker : pos_ticker_list) {
//     const auto pos = portfolio_.get_position(ticker);
//     if (pos.ticker.empty())
//       continue;
//     auto& lp = pos.long_pos;
//     auto& sp = pos.short_pos;
//     spdlog::info("[TradingSystem] [Position] Ticker: {}, "
//                  "LP: {}, LOP: {}, LCP: {}, LongPrice: {:.2f}, LongPNL:
//                  {:.2f}, " "SP: {}, SOP: {}, SCP: {}, ShortPrice: {:.2f},
//                  ShortPNL: {:.2f}", pos.ticker, lp.volume, lp.open_pending,
//                  lp.close_pending, lp.cost_price, lp.pnl, sp.volume,
//                  sp.open_pending, sp.close_pending, sp.cost_price, sp.pnl);
//   }
// }

void TradingSystem::on_tick(cppex::Any* data) {
  std::unique_lock<std::mutex> lock(mutex_);
  auto* tick = data->fetch<TickData>().release();

  TickDB* db;
  auto db_iter = tick_center_.find(tick->ticker);
  if (db_iter == tick_center_.end()) {
    auto res = tick_center_.emplace(tick->ticker, TickDB(tick->ticker));
    res.first->second.process_tick(tick);
  } else {
    db_iter->second.process_tick(tick);
  }

  panel_.update_float_pnl(tick->ticker, tick->last_price);
}

void TradingSystem::on_position(cppex::Any* data) {
  std::unique_lock<std::mutex> lock(mutex_);
  if (is_process_pos_done_) return;

  const auto* position = data->cast<Position>();
  auto& lp = position->long_pos;
  auto& sp = position->short_pos;
  spdlog::info(
      "[TradingSystem] on_position. Query position success. Ticker: {}, "
      "Long Volume: {}, Long Price: {:.2f}, Long Frozen: {}, Long PNL: {}, "
      "Short Volume: {}, Short Price: {:.2f}, Short Frozen: {}, Short PNL: {}",
      position->ticker, lp.volume, lp.cost_price, lp.frozen, lp.float_pnl,
      sp.volume, sp.cost_price, sp.frozen, sp.float_pnl);

  if (lp.volume == 0 && lp.frozen == 0 && sp.volume == 0 && sp.frozen == 0)
    return;

  panel_.process_position(position);
}

void TradingSystem::on_account(cppex::Any* data) {
  std::unique_lock<std::mutex> lock(mutex_);
  auto* account = data->cast<Account>();
  panel_.process_account(account);
  spdlog::info(
      "[StrategyEngine] on_account. Account ID: {}, Balance: {}, Fronzen: {}",
      account->account_id, account->balance, account->frozen);
}

void TradingSystem::on_order(cppex::Any* data) {
  std::unique_lock<std::mutex> lock(mutex_);
  const auto* order = data->cast<Order>();
  panel_.update_order(order);
}

// TODO(Kevin): fix incorrect calculation and missing data
void TradingSystem::on_trade(cppex::Any* data) {
  std::unique_lock<std::mutex> lock(mutex_);
  auto* trade = data->cast<Trade>();
  spdlog::debug(
      "[TradingSystem] on_trade. Ticker: {}, Order ID: {}, Trade ID: {}, "
      "Direction: {}, Offset: {}, Price: {:.2f}, Volume: {}",
      trade->ticker, trade->order_id, trade->trade_id,
      to_string(trade->direction), to_string(trade->offset), trade->price,
      trade->volume);

  panel_.process_new_trade(trade);
  panel_.update_pos_traded(trade->ticker, trade->direction, trade->offset,
                           trade->volume, trade->price);
}

void TradingSystem::on_sync(cppex::Any*) { is_process_pos_done_ = true; }

}  // namespace ft
