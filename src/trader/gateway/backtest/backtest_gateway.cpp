// Copyright [2020-2021] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "trader/gateway/backtest/backtest_gateway.h"

#include <fstream>

#include "ft/base/contract_table.h"
#include "ft/base/log.h"
#include "ft/utils/string_utils.h"

namespace ft {

bool BacktestGateway::Init(const GatewayConfig& config) {
  if (!LoadHistoryData(config.arg0)) {
    return false;
  }

  match_engine_ = CreateMatchEngine(config.arg1);
  if (!match_engine_) {
    LOG_ERROR("match engine {} not found", config.arg1);
    return false;
  }
  match_engine_->RegisterListener(this);
  if (!match_engine_->Init()) {
    return false;
  }
  return true;
}

bool BacktestGateway::SendOrder(const OrderRequest& order, uint64_t* privdata_ptr) {
  *privdata_ptr = order.contract->ticker_id;
  return match_engine_->InsertOrder(order);
}

bool BacktestGateway::CancelOrder(uint64_t order_id, uint64_t privdata) {
  return match_engine_->CancelOrder(order_id, static_cast<uint32_t>(privdata));
}

bool BacktestGateway::Subscribe(const std::vector<std::string>& sub_list) { return true; }

bool BacktestGateway::QueryPositions() {
  OnQueryPositionEnd();
  return true;
}

bool BacktestGateway::QueryAccount() {
  Account account{};
  account.total_asset = account.cash = 1000000000.0;
  OnQueryAccount(account);
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

void BacktestGateway::OnNotify(uint64_t signal) {
  auto& tick = history_data_[tick_cursor_];
  match_engine_->OnNewTick(tick);
  OnTick(tick);
  ++tick_cursor_;
}

void BacktestGateway::OnAccepted(OrderAcceptedRsp* rsp) { OnOrderAccepted(*rsp); }

void BacktestGateway::OnRejected(OrderRejectedRsp* rsp) { OnOrderRejected(*rsp); }

void BacktestGateway::OnTraded(OrderTradedRsp* rsp) { OnOrderTraded(*rsp); }

void BacktestGateway::OnCanceled(OrderCanceledRsp* rsp) { OnOrderCanceled(*rsp); }

void BacktestGateway::OnCancelRejected(OrderCancelRejectedRsp* rsp) { OnOrderCancelRejected(*rsp); }

bool BacktestGateway::LoadHistoryData(const std::string& history_data_file) {
  std::ifstream tick_ifs(history_data_file);
  if (!tick_ifs) {
    LOG_ERROR("tick file not found {}", history_data_file);
    return false;
  }
  std::string line;
  std::vector<std::string> tokens;
  std::getline(tick_ifs, line);
  while (std::getline(tick_ifs, line)) {
    StringSplit(line, ",", &tokens, false);
    if (tokens.size() != 15) {
      LOG_ERROR("invalid tick file {}");
      exit(1);
    }
    history_data_.emplace_back(TickData{});

    auto& tick = history_data_.back();
    tick.local_timestamp_us = 0;
    // tick.exchange_timestamp_us = 0;
    tick.last_price = tokens[1].empty() ? 0.0 : std::stod(tokens[1]);
    tick.ask[0] = tokens[3].empty() ? 0.0 : std::stod(tokens[3]);
    tick.ask_volume[0] = tokens[4].empty() ? 0 : std::stoi(tokens[4]);
    tick.bid[0] = tokens[5].empty() ? 0.0 : std::stoi(tokens[5]);
    tick.bid_volume[0] = tokens[6].empty() ? 0 : std::stoi(tokens[6]);
    tick.highest_price = tokens[7].empty() ? 0.0 : std::stod(tokens[7]);
    tick.lowest_price = tokens[8].empty() ? 0.0 : std::stod(tokens[8]);
    tick.volume = tokens[9].empty() ? 0 : std::stoul(tokens[9]);
    tick.open_interest = tokens[11].empty() ? 0 : std::stoul(tokens[11]);
    auto* contract = ContractTable::get_by_ticker(tokens[13]);
    if (!contract) {
      LOG_ERROR("ticker {} not found", tokens[13]);
      return false;
    }
    tick.ticker_id = contract->ticker_id;

    tokens.clear();
  }

  return true;
}

}  // namespace ft
