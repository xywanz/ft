// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "trading_system/trading_engine.h"

#include "core/contract_table.h"
#include "core/error_code.h"
#include "core/protocol.h"
#include "utils/misc.h"

namespace ft {

TradingEngine::TradingEngine() : portfolio_("127.0.0.1", 6379) {
  risk_mgr_ =
      std::make_unique<RiskManager>(&account_, &portfolio_, &order_map_);
}

TradingEngine::~TradingEngine() { close(); }

bool TradingEngine::login(const Config& config) {
  if (is_logon_) return true;

  config.show();

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

  proto_.set_account(account_.account_id);

  // 启动个线程去定时查询资金账户信息
  std::thread([this]() {
    for (;;) {
      std::this_thread::sleep_for(std::chrono::seconds(15));
      gateway_->query_account();
    }
  }).detach();

  spdlog::info("[TradingEngine::login] Init done");

  is_logon_ = true;
  return true;
}

void TradingEngine::run() {
  spdlog::info("[TradingEngine::run] Start to recv order req from topic: {}",
               proto_.trader_cmd_topic());

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
  auto contract = ContractTable::get_by_index(cmd->order_req.ticker_index);
  if (!contract) {
    spdlog::error("[TradingEngine::send_order] Contract not found");
    return false;
  }

  Order order{};
  auto& req = order.req;
  req.engine_order_id = next_engine_order_id();
  req.ticker_index = cmd->order_req.ticker_index;
  req.direction = cmd->order_req.direction;
  req.offset = cmd->order_req.offset;
  req.volume = cmd->order_req.volume;
  req.type = cmd->order_req.type;
  req.price = cmd->order_req.price;
  order.user_order_id = cmd->order_req.user_order_id;
  order.contract = contract;
  order.status = OrderStatus::SUBMITTING;
  order.strategy_id = cmd->strategy_id;

  std::unique_lock<std::mutex> lock(mutex_);
  int error_code = risk_mgr_->check_order_req(&order);
  if (error_code != NO_ERROR) {
    spdlog::error("[TradingEngine::send_order] 风控未通过: {}",
                  error_code_str(error_code));
    risk_mgr_->on_order_rejected(&order, error_code);
    return false;
  }

  if (!gateway_->send_order(&req)) {
    spdlog::error(
        "[StrategyEngine::send_order] Failed to send_order. Order: <Ticker: "
        "{}, Direction: {}, Offset: {}, OrderType: {}, Traded: {}, Total: {}, "
        "Price: {:.2f}, Status: Failed>",
        contract->ticker, direction_str(req.direction), offset_str(req.offset),
        ordertype_str(req.type), 0, req.volume, req.price);

    risk_mgr_->on_order_rejected(&order, ERR_SEND_FAILED);
    return false;
  }

  order_map_.emplace((uint64_t)req.engine_order_id, order);
  risk_mgr_->on_order_sent(&order);

  spdlog::debug(
      "[StrategyEngine::send_order] Success. Order: <Ticker: {}, TEOrderID: "
      "{}, "
      "Direction: {}, Offset: {}, OrderType: {}, Traded: {}, Total: {}, Price: "
      "{:.2f}, Status: {}>",
      contract->ticker, req.engine_order_id, direction_str(req.direction),
      offset_str(req.offset), ordertype_str(req.type), 0, req.volume, req.price,
      to_string(order.status));
  return true;
}

void TradingEngine::cancel_order(uint64_t order_id) {
  gateway_->cancel_order(order_id);
}

void TradingEngine::cancel_for_ticker(uint32_t ticker_index) {
  std::unique_lock<std::mutex> lock(mutex_);
  for (const auto& [engine_order_id, order] : order_map_) {
    UNUSED(engine_order_id);
    if (ticker_index == order.contract->index)
      gateway_->cancel_order(order.order_id);
  }
}

void TradingEngine::cancel_all() {
  std::unique_lock<std::mutex> lock(mutex_);
  for (const auto& [engine_order_id, order] : order_map_) {
    UNUSED(engine_order_id);
    gateway_->cancel_order(order.order_id);
  }
}

void TradingEngine::on_query_contract(const Contract* contract) {}

void TradingEngine::on_query_account(const Account* account) {
  std::unique_lock<std::mutex> lock(mutex_);
  account_ = *account;
  lock.unlock();

  spdlog::info(
      "[TradingEngine::on_query_account] balance:{:.3f}, frozen:{:.3f}, "
      "margin:{:.3f}",
      account->balance, account->frozen, account->margin);
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
  assert(contract);

  tick_redis_.publish(proto_.quote_key(contract->ticker), tick,
                      sizeof(TickData));
  spdlog::trace("[TradingEngine::process_tick] ask:{:.3f}  bid:{:.3f}",
                tick->ask[0], tick->bid[0]);
}

void TradingEngine::on_query_trade(const Trade* trade) {
  if (is_logon_) {
    spdlog::error("[TradingEngine::on_query_trade] 只能在初始化时查询交易");
    return;
  }

  portfolio_.update_on_query_trade(trade->ticker_index, trade->direction,
                                   trade->offset, trade->volume);
}

/*
 * 订单被市场接受后通知策略
 * 告知策略order_id，策略可通过此order_id撤单
 */
void TradingEngine::on_order_accepted(uint64_t engine_order_id,
                                      uint64_t order_id) {
  std::unique_lock<std::mutex> lock(mutex_);
  auto iter = order_map_.find(engine_order_id);
  if (iter == order_map_.end()) {
    spdlog::warn(
        "[TradingEngine::on_order_accepted] Order not found. OrderID: {}",
        engine_order_id);
    return;
  }

  auto& order = iter->second;
  if (order.accepted) return;

  order.order_id = order_id;
  order.accepted = true;
  risk_mgr_->on_order_accepted(&order);

  spdlog::info(
      "[TradingEngine::on_order_accepted] 报单委托成功. Ticker: {}, Direction: "
      "{}, Offset: {}, Volume: {}, Price: {:.2f}",
      order.contract->ticker, direction_str(order.req.direction),
      offset_str(order.req.offset), order.req.volume, order.req.price);
}

void TradingEngine::on_order_rejected(uint64_t engine_order_id) {
  std::unique_lock<std::mutex> lock(mutex_);
  auto iter = order_map_.find(engine_order_id);
  if (iter == order_map_.end()) {
    spdlog::warn(
        "[TradingEngine::on_order_rejected] Order not found. OrderID: {}",
        engine_order_id);
    return;
  }

  auto& order = iter->second;
  risk_mgr_->on_order_rejected(&order, ERR_REJECTED);

  spdlog::error(
      "[TradingEngine::on_order_rejected] 报单被拒. Ticker: {}, Direction: "
      "{}, Offset: {}, Volume: {}, Price: {:.2f}",
      order.contract->ticker, direction_str(order.req.direction),
      offset_str(order.req.offset), order.req.volume, order.req.price);

  order_map_.erase(iter);
}

void TradingEngine::on_order_traded(uint64_t engine_order_id, uint64_t order_id,
                                    int this_traded, double traded_price) {
  std::unique_lock<std::mutex> lock(mutex_);
  auto iter = order_map_.find(engine_order_id);
  if (iter == order_map_.end()) {
    spdlog::warn(
        "[TradingEngine::on_order_traded] Order not found. OrderID: {}, "
        "Traded: {}, Price: {}",
        engine_order_id, this_traded, traded_price);
    return;
  }

  auto& order = iter->second;
  if (!order.accepted) {
    order.accepted = true;
    risk_mgr_->on_order_accepted(&order);

    spdlog::info(
        "[TradingEngine::on_order_accepted] 报单委托成功. Ticker: {}, "
        "Direction: {}, Offset: {}, Volume: {}, Price: {:.2f}",
        order.contract->ticker, direction_str(order.req.direction),
        offset_str(order.req.offset), order.req.volume, order.req.price);
  }

  order.order_id = order_id;
  order.traded_volume += this_traded;

  spdlog::info(
      "[TradingEngine::on_order_traded] 报单成交. Ticker: {}, Direction: {}, "
      "Offset: {}, Traded: {}, Price: {}",
      order.contract->ticker, direction_str(order.req.direction),
      offset_str(order.req.offset), this_traded, traded_price);

  risk_mgr_->on_order_traded(&order, this_traded, traded_price);

  if (order.traded_volume + order.canceled_volume == order.req.volume) {
    spdlog::info(
        "[TradingEngine::on_order_traded] 报单完成. Ticker: {}, Direction: {}, "
        "Offset: {}, Traded/Original: {}/{}",
        order.contract->ticker, direction_str(order.req.direction),
        offset_str(order.req.offset), order.traded_volume, order.req.volume);

    // 订单结束，通知风控模块
    risk_mgr_->on_order_completed(&order);
    order_map_.erase(iter);
  }
}

void TradingEngine::on_order_canceled(uint64_t engine_order_id,
                                      int canceled_volume) {
  std::unique_lock<std::mutex> lock(mutex_);
  auto iter = order_map_.find(engine_order_id);
  if (iter == order_map_.end()) {
    spdlog::warn(
        "[TradingEngine::on_order_canceled] Order not found. OrderID: {}",
        engine_order_id);
    return;
  }

  auto& order = iter->second;
  order.canceled_volume = canceled_volume;

  spdlog::info(
      "[TradingEngine::on_order_canceled] 报单已撤. Ticker: {}, Direction: {}, "
      "Offset: {}, Canceled: {}",
      order.contract->ticker, direction_str(order.req.direction),
      offset_str(order.req.offset), canceled_volume);

  risk_mgr_->on_order_canceled(&order, canceled_volume);

  if (order.traded_volume + order.canceled_volume == order.req.volume) {
    spdlog::info(
        "[TradingEngine::on_order_canceled] 报单完成. Ticker: {}, Direction: "
        "{}, Offset: {}, Traded/Original: {}/{}",
        order.contract->ticker, direction_str(order.req.direction),
        offset_str(order.req.offset), order.traded_volume, order.req.volume);

    risk_mgr_->on_order_completed(&order);
    order_map_.erase(iter);
  }
}

void TradingEngine::on_order_cancel_rejected(uint64_t engine_order_id) {
  spdlog::warn(
      "[TradingEngine::on_order_cancel_rejected] Order cannot be canceled. "
      "OrderID: {}",
      engine_order_id);
}

}  // namespace ft
