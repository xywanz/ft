// Copyright [2020-2021] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "trader/gateway/backtest/backtest_gateway.h"

#include <cassert>
#include <fstream>

#include "ft/base/contract_table.h"
#include "ft/base/log.h"
#include "ft/utils/misc.h"
#include "ft/utils/protocol_utils.h"
#include "ft/utils/string_utils.h"

namespace ft {

bool BacktestGateway::Init(const GatewayConfig& config) {
  if (!LoadMatchEngine(config.extended_args)) {
    return false;
  }

  current_ticks_.resize(ContractTable::size() + 1);
  if (!LoadDataFeed(config.extended_args)) {
    return false;
  }

  Account init_fund{};
  init_fund.total_asset = 100000000.0;
  init_fund.balance = init_fund.total_asset;
  init_fund.cash = init_fund.total_asset;
  init_fund.margin = 0.0;
  init_fund.frozen = 0.0;
  init_fund.floating_pnl = 0.0;
  if (!fund_calculator_.Init(init_fund)) {
    LOG_ERROR("init fund calculator failed");
    return false;
  }

  return true;
}

bool BacktestGateway::SendOrder(const OrderRequest& order, uint64_t* privdata_ptr) {
  std::unique_lock<SpinLock> lock(spinlock_);
  if (!CheckOrder(order)) {
    return false;
  }

  fund_calculator_.UpdatePending(order, order.volume);
  pos_calculator_.UpdatePending(order.contract->ticker_id, order.direction, order.offset,
                                order.volume);

  *privdata_ptr = order.contract->ticker_id;
  if (!match_engine_->InsertOrder(order)) {
    fund_calculator_.UpdatePending(order, -order.volume);
    pos_calculator_.UpdatePending(order.contract->ticker_id, order.direction, order.offset,
                                  -order.volume);
    return false;
  }
  return true;
}

bool BacktestGateway::CancelOrder(uint64_t order_id, uint64_t privdata) {
  std::unique_lock<SpinLock> lock(spinlock_);
  return match_engine_->CancelOrder(order_id, static_cast<uint32_t>(privdata));
}

bool BacktestGateway::Subscribe(const std::vector<std::string>& sub_list) { return true; }

bool BacktestGateway::QueryPositions() {
  OnQueryPositionEnd();
  return true;
}

bool BacktestGateway::QueryAccount() {
  OnQueryAccount(fund_calculator_.GetFundAccount());
  OnQueryAccountEnd();
  return true;
}

bool BacktestGateway::QueryTrades() {
  OnQueryTradeEnd();
  return true;
}

bool BacktestGateway::QueryOrders() {
  OnQueryOrderEnd();
  return true;
}

void BacktestGateway::OnNotify(uint64_t signal) { data_feed_->Feed(); }

void BacktestGateway::OnAccepted(const OrderRequest& order) {
  OrderAcceptedRsp rsp{order.order_id};
  OnOrderAccepted(rsp);
}

void BacktestGateway::OnRejected(const OrderRequest& order) {
  fund_calculator_.UpdatePending(order, -order.volume);
  pos_calculator_.UpdatePending(order.contract->ticker_id, order.direction, order.offset,
                                -order.volume);

  OrderRejectedRsp rsp{order.order_id};
  OnOrderRejected(rsp);
}

void BacktestGateway::OnTraded(const OrderRequest& order, int volume, double price,
                               uint64_t timestamp_us) {
  fund_calculator_.UpdateTraded(order, volume, price, current_ticks_[order.contract->ticker_id]);
  pos_calculator_.UpdateTraded(order.contract->ticker_id, order.direction, order.offset, volume,
                               price);

  OrderTradedRsp rsp{};
  rsp.timestamp_us = timestamp_us;
  rsp.order_id = order.order_id;
  rsp.volume = volume;
  rsp.price = price;
  OnOrderTraded(rsp);
}

void BacktestGateway::OnCanceled(const OrderRequest& order, int canceled_volume) {
  fund_calculator_.UpdatePending(order, -canceled_volume);
  pos_calculator_.UpdatePending(order.contract->ticker_id, order.direction, order.offset,
                                -canceled_volume);

  OrderCanceledRsp rsp{order.order_id, canceled_volume};
  OnOrderCanceled(rsp);
}

void BacktestGateway::OnCancelRejected(uint64_t order_id) {
  OrderCancelRejectedRsp rsp{order_id};
  OnOrderCancelRejected(rsp);
}

void BacktestGateway::OnDataFeed(TickData* tick) {
  std::unique_lock<SpinLock> lock(spinlock_);
  auto* pos = pos_calculator_.GetPosition(tick->ticker_id);
  if (pos && (pos->long_pos.holdings > 0 || pos->short_pos.holdings > 0)) {
    fund_calculator_.UpdatePrice(*pos, current_ticks_[tick->ticker_id], *tick);
  }
  current_ticks_[tick->ticker_id] = *tick;

  match_engine_->OnNewTick(*tick);
  OnTick(*tick);
}

bool BacktestGateway::LoadMatchEngine(const std::map<std::string, std::string>& args) {
  std::string match_engine_name = kDefaultMatchEngine;
  auto match_engine_it = args.find("match_engine");
  if (match_engine_it != args.end()) {
    match_engine_name = match_engine_it->second;
    LOG_INFO("find match engine config: {}", match_engine_name);
  } else {
    LOG_INFO("use default match engine: {}", match_engine_name);
  }
  match_engine_ = CreateMatchEngine(match_engine_name);
  if (!match_engine_) {
    LOG_ERROR("match engine {} not found", match_engine_name);
    return false;
  }
  match_engine_->RegisterListener(this);
  if (!match_engine_->Init()) {
    LOG_ERROR("match engine {} init failed", match_engine_name);
    return false;
  }
  LOG_INFO("match engine {} inited", match_engine_name);
  return true;
}

bool BacktestGateway::LoadDataFeed(const std::map<std::string, std::string>& args) {
  std::string data_feed_name;
  auto feed_name_it = args.find("data_feed");
  if (feed_name_it == args.end()) {
    LOG_ERROR("option data_feed not indicated");
    return false;
  } else {
    data_feed_name = feed_name_it->second;
    LOG_INFO("use data feed: {}", data_feed_name);
  }
  data_feed_ = CreateDataFeed(data_feed_name);
  if (!data_feed_) {
    LOG_ERROR("data feed {} not found", data_feed_name);
    return false;
  }
  data_feed_->RegisterListener(this);
  if (!data_feed_->Init(args)) {
    LOG_ERROR("data feed {} init failed", data_feed_name);
    return false;
  }
  LOG_INFO("data feed {} inited", data_feed_name);
  return true;
}

bool BacktestGateway::CheckOrder(const OrderRequest& order) const {
  // 不支持市价单
  if (order.type != OrderType::kLimit && order.type != OrderType::kBest &&
      order.type != OrderType::kFak && order.type != OrderType::kFok) {
    LOG_ERROR("unsupported order type: {}", ToString(order.type));
    return false;
  }

  if (order.price <= 0.0 + 1e-5) {
    LOG_ERROR("invalid price: {}", order.price);
    return false;
  }

  if (order.volume <= 0) {
    LOG_ERROR("invalid volume: {}", order.volume);
    return false;
  }

  if (!fund_calculator_.CheckFund(order)) {
    return false;
  }

  if (IsOffsetClose(order.offset)) {
    int available = 0;
    const auto* pos = pos_calculator_.GetPosition(order.contract->ticker_id);
    if (pos) {
      const auto& detail = order.direction == Direction::kSell ? pos->long_pos : pos->short_pos;
      available = detail.holdings - detail.close_pending;
    }

    if (available < order.volume) {
      LOG_ERROR("position not enough. available:{} to_close:{}", available, order.volume);
      return false;
    }
  }

  return true;
}

}  // namespace ft
