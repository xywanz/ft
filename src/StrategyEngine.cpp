// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "AlgoTrade/StrategyEngine.h"

#include <spdlog/spdlog.h>

#include <cassert>
#include <set>
#include <vector>

#include "AlgoTrade/Strategy.h"
#include "Base/DataStruct.h"
#include "ContractTable.h"
#include "RiskManagement/NoSelfTrade.h"
#include "RiskManagement/VelocityLimit.h"

namespace ft {

StrategyEngine::StrategyEngine() : engine_(new EventEngine) {
  engine_->set_handler(EV_ACCOUNT,
                       MEM_HANDLER(StrategyEngine::process_account));
  engine_->set_handler(EV_POSITION,
                       MEM_HANDLER(StrategyEngine::process_position));
  engine_->set_handler(EV_ORDER, MEM_HANDLER(StrategyEngine::process_order));
  engine_->set_handler(EV_TRADE, MEM_HANDLER(StrategyEngine::process_trade));
  engine_->set_handler(EV_TICK, MEM_HANDLER(StrategyEngine::process_tick));
  engine_->set_handler(EV_MOUNT_STRATEGY,
                       MEM_HANDLER(StrategyEngine::process_mount_strategy));
  engine_->set_handler(EV_UMOUNT_STRATEGY,
                       MEM_HANDLER(StrategyEngine::process_unmount_strategy));
  engine_->set_handler(EV_SYNC, MEM_HANDLER(StrategyEngine::process_sync));

  risk_mgr_.add_rule(std::make_shared<NoSelfTradeRule>(&panel_));
  risk_mgr_.add_rule(std::make_shared<VelocityLimit>(1000, 10, 200));
}

StrategyEngine::~StrategyEngine() {}

void StrategyEngine::close() {
  is_logon_ = false;
  for (auto& [ticker, strategy_list] : strategies_) {
    for (auto strategy : strategy_list) unmount_strategy(strategy);
  }
  gateway_->logout();
  engine_->stop();
}

bool StrategyEngine::login(const LoginParams& params) {
  if (is_logon_) return true;

  gateway_.reset(create_gateway("ctp", engine_.get()));
  if (!gateway_) {
    spdlog::error("[StrategyEngine::login] Failed. Unknown gateway");
    return false;
  }

  if (!gateway_->login(params)) {
    spdlog::error("[StrategyEngine::login] Failed to login");
    return false;
  }

  spdlog::info("[StrategyEngine::login] Success. Login as {}",
               params.investor_id());

  engine_->run(false);

  if (!gateway_->query_account()) {
    spdlog::error("[StrategyEngine::login] Failed to query account");
    return false;
  }

  // query all positions
  if (!gateway_->query_positions()) {
    spdlog::error("[StrategyEngine::login] Failed to query positions");
    return false;
  }

  engine_->post(EV_SYNC);
  while (!is_logon_) continue;

  return true;
}

uint64_t StrategyEngine::send_order(const std::string& ticker, int volume,
                                    Direction direction, Offset offset,
                                    OrderType type, double price) {
  if (!is_logon_) {
    spdlog::error("[StrategyEngine::send_order] Failed. Not logon");
    return 0;
  }

  const auto* contract = ContractTable::get_by_ticker(ticker);
  if (!contract) {
    spdlog::error("[StrategyEngine::send_order] Contract not found");
    return 0;
  }

  Order order(contract->index, direction, offset, volume, type, price);
  order.status = OrderStatus::SUBMITTING;

  if (!risk_mgr_.check(&order)) {
    spdlog::error("[StrategyEngine::send_order] RiskMgr check failed");
    return 0;
  }

  order.order_id = gateway_->send_order(&order);
  if (order.order_id == 0) {
    spdlog::error(
        "[StrategyEngine::send_order] Failed to send_order."
        " Order: <Ticker: {}, OrderID: {}, Direction: {}, "
        "Offset: {}, OrderType: {}, Traded: {}, Total: {}, Price: {:.2f}, "
        "Status: {}>",
        contract->ticker, order.order_id, to_string(order.direction),
        to_string(order.offset), to_string(order.type), 0, order.volume,
        order.price, to_string(order.status));
    return 0;
  }

  panel_.process_new_order(&order);
  panel_.update_pos_pending(contract->index, direction, offset, volume);

  spdlog::debug(
      "[StrategyEngine::send_order] Success."
      " Order: <Ticker: {}, OrderID: {}, Direction: {}, "
      "Offset: {}, OrderType: {}, Traded: {}, Total: {}, Price: {:.2f}, "
      "Status: {}>",
      contract->ticker, order.order_id, to_string(order.direction),
      to_string(order.offset), to_string(order.type), 0, order.volume,
      order.price, to_string(order.status));
  return order.order_id;
}

bool StrategyEngine::cancel_order(uint64_t order_id) {
  if (!is_logon_) {
    spdlog::error("[StrategyEngine::send_order] Failed. Not logon");
    return false;
  }

  const Order* order = panel_.get_order_by_id(order_id);
  if (!order) {
    spdlog::error(
        "[StrategyEngine::cancel_order] Failed. Order not found. OrderID: {}",
        order_id);
    return false;
  }

  if (!gateway_->cancel_order(order_id)) {
    spdlog::error("[StrategyEngine::cancel_order] Failed");
    return false;
  }

  spdlog::debug(
      "[StrategyEngine::cancel_order] Canceling."
      " Order: <OrderID: {}, Direction: {}, "
      "Offset: {}, OrderType: {}, Traded: {}, Total: {}, Price: {:.2f}, "
      "Status: {}>",
      order->order_id, to_string(order->direction), to_string(order->offset),
      to_string(order->type), order->volume_traded, order->volume, order->price,
      to_string(order->status));
  return true;
}

bool StrategyEngine::mount_strategy(const std::string& ticker,
                                    Strategy* strategy) {
  if (!is_logon_) {
    spdlog::error("[StrategyEngine::mount_strategy] Not logon");
    return false;
  }

  strategy->set_ctx(new AlgoTradeContext(ticker, this, &data_center_, &panel_));
  engine_->post(EV_MOUNT_STRATEGY, strategy);
  return true;
}

void StrategyEngine::process_mount_strategy(cppex::Any* data) {
  auto* strategy = data->fetch<Strategy>().release();

  const auto& ticker = strategy->get_ctx()->this_ticker();
  const auto* contract = ContractTable::get_by_ticker(ticker);
  assert(contract);
  auto& list = strategies_[contract->index];

  if (strategy->on_init(strategy->get_ctx()))
    list.emplace_back(strategy);
  else
    engine_->post(EV_MOUNT_STRATEGY, strategy);
}

void StrategyEngine::unmount_strategy(Strategy* strategy) {
  engine_->post(EV_UMOUNT_STRATEGY, strategy);
}

void StrategyEngine::process_unmount_strategy(cppex::Any* data) {
  auto* strategy = data->fetch<Strategy>().release();

  auto ctx = strategy->get_ctx();
  if (!ctx) return;

  const auto& ticker = strategy->get_ctx()->this_ticker();
  const auto* contract = ContractTable::get_by_ticker(ticker);
  assert(contract);
  auto iter = strategies_.find(contract->index);
  if (iter == strategies_.end()) return;

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

void StrategyEngine::process_sync(cppex::Any*) { is_logon_ = true; }

void StrategyEngine::process_tick(cppex::Any* data) {
  auto* tick = data->cast<TickData>();

  const auto* contract = ContractTable::get_by_index(tick->ticker_index);
  if (!contract) {
    spdlog::error("[StrategyEngine::process_tick] Contract not found");
    return;
  }

  data_center_.process_tick(tick);
  panel_.update_float_pnl(contract->index, tick->last_price);

  auto iter = strategies_.find(contract->index);
  if (iter != strategies_.end()) {
    auto& strategy_list = iter->second;
    for (auto& strategy : strategy_list) strategy->on_tick(strategy->get_ctx());
  }
}

void StrategyEngine::process_position(cppex::Any* data) {
  const auto* position = data->cast<Position>();

  const auto* contract = ContractTable::get_by_index(position->ticker_index);
  if (!contract) {
    spdlog::error("[StrategyEngine::process_position] Contract not found");
    return;
  }

  auto& lp = position->long_pos;
  auto& sp = position->short_pos;
  spdlog::info(
      "[StrategyEngine::process_position] Ticker: {}, "
      "Long Volume: {}, Long Price: {:.2f}, Long Frozen: {}, Long PNL: {}, "
      "Short Volume: {}, Short Price: {:.2f}, Short Frozen: {}, Short PNL: {}",
      contract->ticker, lp.volume, lp.cost_price, lp.frozen, lp.float_pnl,
      sp.volume, sp.cost_price, sp.frozen, sp.float_pnl);

  if (lp.volume == 0 && lp.frozen == 0 && sp.volume == 0 && sp.frozen == 0)
    return;

  panel_.process_position(position);
}

void StrategyEngine::process_account(cppex::Any* data) {
  auto* account = data->cast<Account>();
  panel_.process_account(account);
  spdlog::info(
      "[StrategyEngine::process_account] Account ID: {}, Balance: {}, Fronzen: "
      "{}",
      account->account_id, account->balance, account->frozen);
}

void StrategyEngine::process_trade(cppex::Any* data) {
  auto* trade = data->cast<Trade>();

  const auto* contract = ContractTable::get_by_index(trade->ticker_index);
  if (!contract) {
    spdlog::error("[StrategyEngine::process_trade] Contract not found");
    return;
  }

  spdlog::debug(
      "[StrategyEngine::process_trade] Ticker: {}, Order ID: {}, Trade ID: {}, "
      "Direction: {}, Offset: {}, Price: {:.2f}, Volume: {}",
      contract->ticker, trade->order_id, trade->trade_id,
      to_string(trade->direction), to_string(trade->offset), trade->price,
      trade->volume);

  panel_.process_new_trade(trade);
  panel_.update_pos_traded(contract->index, trade->direction, trade->offset,
                           trade->volume, trade->price);
}

void StrategyEngine::process_order(cppex::Any* data) {
  const auto* order = data->cast<Order>();
  panel_.update_order(order);

  // TODO(kevin): 对策略发的单加个ID，只把订单回执返回给发该单的策略
  for (auto& [ticker, strategy_list] : strategies_) {
    for (auto strategy : strategy_list)
      strategy->on_order(strategy->get_ctx(), order);
  }
}

}  // namespace ft
