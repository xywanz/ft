// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "TradingSystem/TradingEngine.h"

#include "Core/ContractTable.h"
#include "Core/Protocol.h"
#include "RiskManagement/RiskManager.h"
#include "Utils/Misc.h"

namespace ft {

TradingEngine::TradingEngine()
    : portfolio_("127.0.0.1", 6379),
      risk_mgr_(std::make_unique<RiskManager>(&portfolio_)) {}

TradingEngine::~TradingEngine() { close(); }

bool TradingEngine::login(const Config& config) {
  if (is_logon_) return true;

  gateway_.reset(create_gateway(config.api, this));
  if (!gateway_) {
    spdlog::error("[TradingEngine::login] Failed. Unknown gateway");
    return false;
  }

  if (!gateway_->login(config)) {
    spdlog::error("[TradingEngine::login] Failed to login");
    return false;
  }

  spdlog::info("[TradingEngine::login] Success. Login as {}",
               config.investor_id);

  spdlog::info("[[TradingEngine::login] Querying account");
  if (!gateway_->query_account()) {
    spdlog::error("[TradingEngine::login] Failed to query account");
    return false;
  }
  spdlog::info("[[TradingEngine::login] Querying account done");

  // query all positions
  spdlog::info("[[TradingEngine::login] Querying positions");
  portfolio_.init(account_.account_id);
  if (!gateway_->query_positions()) {
    spdlog::error("[TradingEngine::login] Failed to query positions");
    return false;
  }
  spdlog::info("[[TradingEngine::login] Querying positions done");

  spdlog::info("[[TradingEngine::login] Querying trades");
  if (!gateway_->query_trades()) {
    spdlog::error("[TradingEngine::login] Failed to query trades");
    return false;
  }
  spdlog::info("[[TradingEngine::login] Querying trades done");

  proto_.set_account_id(account_.account_id);
  spdlog::info("[TradingEngine::login] Init done");

  is_logon_ = true;
  return true;
}

void TradingEngine::run() {
  spdlog::info("[TradingEngine::run] Start to recv order req");

  order_redis_.subscribe({proto_.trader_cmd_topic()});

  for (;;) {
    auto reply = order_redis_.get_sub_reply();
    if (!reply) continue;

    auto cmd = reinterpret_cast<const TraderCommand*>(reply->element[2]->str);
    if (cmd->magic != TRADER_CMD_MAGIC) {
      spdlog::error("[TradingEngine::run] Recv unknown cmd: error magic num");
      continue;
    }

    switch (cmd->type) {
      case NEW_ORDER:
        spdlog::info("new order");
        send_order(cmd);
        break;
      case CANCEL_ORDER:
        spdlog::info("cancel order");
        cancel_order(cmd->cancel_req.order_id);
        break;
      case CANCEL_TICKER:
        spdlog::info("cancel all for ticker");
        cancel_for_ticker(cmd->cancel_ticker_req.ticker_index);
        break;
      case CANCEL_ALL:
        spdlog::info("cancel all");
        cancel_all();
        break;
      default:
        spdlog::error("[StrategyEngine::run] Unknown cmd");
        break;
    }
  }
}

void TradingEngine::close() {
  if (gateway_) gateway_->logout();
}

bool TradingEngine::send_order(const TraderCommand* cmd) {
  if (!is_logon_) {
    spdlog::error("[TradingEngine::send_order] Failed. Not logon");
    return false;
  }

  auto& sreq = cmd->order_req;
  auto contract = ContractTable::get_by_index(sreq.ticker_index);
  if (!contract) {
    spdlog::error("[TradingEngine::send_order] Contract not found");
    return false;
  }

  OrderReq req;
  req.engine_order_id = next_engine_order_id();
  req.ticker_index = sreq.ticker_index;
  req.direction = sreq.direction;
  req.offset = sreq.offset;
  req.volume = sreq.volume;
  req.type = sreq.type;
  req.price = sreq.price;

  std::unique_lock<std::mutex> lock(mutex_);
  if (risk_mgr_) {
    if (!risk_mgr_->check_order_req(&req)) {
      spdlog::error("风控未通过");
      respond_send_order_error(cmd);
      return false;
    }
  }

  uint64_t order_id = gateway_->send_order(&req);
  if (order_id == 0) {
    spdlog::error(
        "[StrategyEngine::send_order] Failed to send_order. Order: <Ticker: "
        "{}, Direction: {}, Offset: {}, OrderType: {}, Traded: {}, Total: {}, "
        "Price: {:.2f}, Status: Failed>",
        contract->ticker, direction_str(req.direction), offset_str(req.offset),
        ordertype_str(req.type), 0, req.volume, req.price);

    if (risk_mgr_) risk_mgr_->on_order_completed(req.engine_order_id);
    respond_send_order_error(cmd);
    return false;
  }

  if (risk_mgr_) risk_mgr_->on_order_sent(req.engine_order_id);

  Order order{};
  order.contract = contract;
  order.engine_order_id = req.engine_order_id;
  order.user_order_id = sreq.user_order_id;
  order.direction = sreq.direction;
  order.offset = sreq.offset;
  order.volume = sreq.volume;
  order.type = sreq.type;
  order.price = sreq.price;
  order.status = OrderStatus::SUBMITTING;
  order.strategy_id = cmd->strategy_id;
  order_map_.emplace(order_id, order);

  portfolio_.update_pending(contract->index, order.direction, order.offset,
                            order.volume);

  spdlog::debug(
      "[StrategyEngine::send_order] Success. Order: <Ticker: {}, OrderID: {}, "
      "Direction: {}, Offset: {}, OrderType: {}, Traded: {}, Total: {}, Price: "
      "{:.2f}, Status: {}>",
      contract->ticker, order_id, direction_str(order.direction),
      offset_str(order.offset), ordertype_str(order.type), 0, order.volume,
      order.price, to_string(order.status));
  return true;
}

void TradingEngine::cancel_order(uint64_t order_id) {
  gateway_->cancel_order(order_id);
}

void TradingEngine::cancel_for_ticker(uint32_t ticker_index) {
  std::unique_lock<std::mutex> lock(mutex_);
  for (const auto& [order_id, order] : order_map_) {
    if (ticker_index == order.contract->index) gateway_->cancel_order(order_id);
  }
}

void TradingEngine::cancel_all() {
  std::unique_lock<std::mutex> lock(mutex_);
  for (const auto& [order_id, order] : order_map_) {
    UNUSED(order);
    gateway_->cancel_order(order_id);
  }
}

void TradingEngine::on_query_contract(const Contract* contract) {}

void TradingEngine::on_query_account(const Account* account) {
  account_ = *account;
  spdlog::info(
      "[TradingEngine::on_query_account] Account ID: {}, Balance: {}, Fronzen: "
      "{}",
      account->account_id, account->balance, account->frozen);
}

void TradingEngine::on_query_position(const Position* position) {
  if (is_logon_) {
    spdlog::error("[TradingEngine::on_query_position] 只能在初始化时查询仓位");
    return;
  }

  auto contract = ContractTable::get_by_index(position->ticker_index);
  assert(contract);

  auto& lp = position->long_pos;
  auto& sp = position->short_pos;
  spdlog::info(
      "[TradingEngine::on_query_position] Ticker: {}, "
      "Long Volume: {}, Long Price: {:.2f}, Long Frozen: {}, Long PNL: {}, "
      "Short Volume: {}, Short Price: {:.2f}, Short Frozen: {}, Short PNL: {}",
      contract->ticker, lp.holdings, lp.cost_price, lp.frozen, lp.float_pnl,
      sp.holdings, sp.cost_price, sp.frozen, sp.float_pnl);

  if (lp.holdings == 0 && lp.frozen == 0 && sp.holdings == 0 && sp.frozen == 0)
    return;

  portfolio_.set_position(position);
}

void TradingEngine::on_tick(const TickData* tick) {
  if (!is_logon_) return;

  auto contract = ContractTable::get_by_index(tick->ticker_index);
  if (!contract) {
    spdlog::error("[TradingEngine::process_tick] Contract not found");
    return;
  }

  tick_redis_.publish(proto_.quote_key(contract->ticker), tick,
                      sizeof(TickData));
  spdlog::debug("[TradingEngine::process_tick]");
}

void TradingEngine::on_query_trade(const Trade* trade) {
  if (is_logon_) {
    spdlog::error("[TradingEngine::on_query_trade] 只能在初始化时查询交易");
    return;
  }

  portfolio_.update_on_query_trade(trade->ticker_index, trade->direction,
                                   trade->offset, trade->volume);
}

void TradingEngine::on_order_accepted(uint64_t order_id) {
  std::unique_lock<std::mutex> lock(mutex_);
  auto iter = order_map_.find(order_id);
  if (iter == order_map_.end()) {
    spdlog::error(
        "[TradingEngine::on_order_accepted] Order not found. OrderID: {}",
        order_id);
    return;
  }

  auto& order = iter->second;

  if (order.strategy_id[0] != 0) {
    OrderResponse rsp{};
    rsp.user_order_id = order.user_order_id;
    rsp.order_id = order_id;
    rsp.ticker_index = order.contract->index;
    rsp.direction = order.direction;
    rsp.offset = order.offset;
    rsp.original_volume = order.volume;
    rsp_redis_.publish(order.strategy_id, &rsp, sizeof(rsp));
  }

  spdlog::info(
      "[TradingEngine::on_order_accepted] 报单委托成功. Ticker: {}, Direction: "
      "{}, Offset: {}, Volume: {}, Price: {:.2f}",
      order.contract->ticker, direction_str(order.direction),
      offset_str(order.offset), order.volume, order.price);
}

void TradingEngine::on_order_rejected(uint64_t order_id) {
  std::unique_lock<std::mutex> lock(mutex_);
  auto iter = order_map_.find(order_id);
  if (iter == order_map_.end()) {
    spdlog::error(
        "[TradingEngine::on_order_rejected] Order not found. OrderID: {}",
        order_id);
    return;
  }

  auto& order = iter->second;

  if (order.strategy_id[0] != 0) {
    OrderResponse rsp{};
    rsp.user_order_id = order.user_order_id;
    rsp.order_id = order_id;
    rsp.ticker_index = order.contract->index;
    rsp.direction = order.direction;
    rsp.offset = order.offset;
    rsp.original_volume = order.volume;
    rsp.completed = true;
    rsp_redis_.publish(order.strategy_id, &rsp, sizeof(rsp));
  }

  spdlog::error(
      "[TradingEngine::on_order_rejected] 报单被拒. Ticker: {}, Direction: "
      "{}, Offset: {}, Volume: {}, Price: {:.2f}",
      order.contract->ticker, direction_str(order.direction),
      offset_str(order.offset), order.volume, order.price);

  order_map_.erase(iter);
}

void TradingEngine::on_order_traded(uint64_t order_id, int this_traded,
                                    double traded_price) {
  std::unique_lock<std::mutex> lock(mutex_);
  auto iter = order_map_.find(order_id);
  if (iter == order_map_.end()) {
    spdlog::error(
        "[TradingEngine::on_order_traded] Order not found. OrderID: {}, "
        "Traded: {}, Price: {}",
        order_id, this_traded, traded_price);
    return;
  }
  auto& order = iter->second;

  spdlog::info(
      "[TradingEngine::on_order_traded] 报单成交. Ticker: {}, Direction: {}, "
      "Offset: {}, Traded: {}, Price: {}",
      order.contract->ticker, direction_str(order.direction),
      offset_str(order.offset), this_traded, traded_price);

  portfolio_.update_traded(order.contract->index, order.direction, order.offset,
                           this_traded, traded_price);

  if (risk_mgr_)
    risk_mgr_->on_order_traded(order.engine_order_id, this_traded,
                               traded_price);

  bool completed = false;
  order.traded_volume += this_traded;
  if (order.traded_volume + order.canceled_volume == order.volume) {
    spdlog::info(
        "[TradingEngine::on_order_traded] 报单完成. Ticker: {}, Direction: {}, "
        "Offset: {}, Traded/Original: {}/{}",
        order.contract->ticker, direction_str(order.direction),
        offset_str(order.offset), order.traded_volume, order.volume);

    // 订单结束，通知风控模块
    if (risk_mgr_) risk_mgr_->on_order_completed(order.engine_order_id);

    completed = true;
    order_map_.erase(iter);
  }

  if (order.strategy_id[0] != 0) {
    OrderResponse rsp{};
    rsp.user_order_id = order.user_order_id;
    rsp.order_id = order_id;
    rsp.ticker_index = order.contract->index;
    rsp.direction = order.direction;
    rsp.offset = order.offset;
    rsp.original_volume = order.volume;
    rsp.traded_volume = order.traded_volume;
    rsp.this_traded = this_traded;
    rsp.this_traded_price = traded_price;
    rsp.completed = completed;
    rsp_redis_.publish(order.strategy_id, &rsp, sizeof(rsp));
  }
}

void TradingEngine::on_order_canceled(uint64_t order_id, int canceled_volume) {
  std::unique_lock<std::mutex> lock(mutex_);
  auto iter = order_map_.find(order_id);
  if (iter == order_map_.end()) {
    spdlog::error(
        "[TradingEngine::on_order_canceled] Order not found. OrderID: {}",
        order_id);
    return;
  }
  auto& order = iter->second;

  spdlog::info(
      "[TradingEngine::on_order_canceled] 报单已撤. Ticker: {}, Direction: {}, "
      "Offset: {}, Canceled: {}",
      order.contract->ticker, direction_str(order.direction),
      offset_str(order.offset), canceled_volume);

  order.canceled_volume = canceled_volume;
  if (order.traded_volume + order.canceled_volume == order.volume) {
    spdlog::info(
        "[TradingEngine::on_order_canceled] 报单完成. Ticker: {}, Direction: "
        "{}, Offset: {}, Traded/Original: {}/{}",
        order.contract->ticker, direction_str(order.direction),
        offset_str(order.offset), order.traded_volume, order.volume);

    // 订单结束，通知风控模块
    if (risk_mgr_) risk_mgr_->on_order_completed(order.engine_order_id);

    if (order.strategy_id[0] != 0) {
      OrderResponse rsp{};
      rsp.user_order_id = order.user_order_id;
      rsp.order_id = order_id;
      rsp.ticker_index = order.contract->index;
      rsp.direction = order.direction;
      rsp.offset = order.offset;
      rsp.original_volume = order.volume;
      rsp.traded_volume = order.traded_volume;
      rsp.completed = true;
      rsp_redis_.publish(order.strategy_id, &rsp, sizeof(rsp));
    }

    order_map_.erase(iter);
  }
}

void TradingEngine::on_order_cancel_rejected(uint64_t order_id) {
  spdlog::warn(
      "[TradingEngine::on_order_cancel_rejected] Order cannot be canceled. "
      "OrderID: {}",
      order_id);
}

void TradingEngine::respond_send_order_error(const TraderCommand* cmd) {
  if (cmd->strategy_id[0] == 0) return;

  OrderResponse rsp{};
  rsp.user_order_id = cmd->order_req.user_order_id;
  rsp.ticker_index = cmd->order_req.ticker_index;
  rsp.direction = cmd->order_req.direction;
  rsp.offset = cmd->order_req.offset;
  rsp.original_volume = cmd->order_req.volume;
  rsp.completed = true;
  rsp_redis_.publish(cmd->strategy_id, &rsp, sizeof(rsp));
}

}  // namespace ft
