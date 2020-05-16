// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "Gateway/Xtp/XtpTradeApi.h"

#include <spdlog/spdlog.h>

#include <cstdlib>

#include "Core/ContractTable.h"

namespace ft {

XtpTradeApi::XtpTradeApi(TradingEngineInterface* engine) : engine_(engine) {}

bool XtpTradeApi::login(const LoginParams& params) {
  if (session_id_ != 0) {
    spdlog::error("[XtpTradeApi::login] Don't login twice");
    return false;
  }

  uint32_t seed = time(nullptr);
  uint8_t client_id = rand_r(&seed) & 0xff;
  trade_api_.reset(XTP::API::TraderApi::CreateTraderApi(client_id, "."));
  if (!trade_api_) {
    spdlog::error("[XtpTradeApi::login] Failed to CreateTraderApi");
    return false;
  }

  char protocol[32]{};
  char ip[32]{};
  int port = 0;

  try {
    int ret = sscanf(params.front_addr().c_str(), "%[^:]://%[^:]:%d", protocol,
                     ip, &port);
    assert(ret == 3);
  } catch (...) {
    assert(false);
  }

  XTP_PROTOCOL_TYPE sock_type = XTP_PROTOCOL_TCP;
  if (strcmp(protocol, "udp") == 0) sock_type = XTP_PROTOCOL_UDP;

  trade_api_->SubscribePublicTopic(XTP_TERT_QUICK);
  trade_api_->RegisterSpi(this);
  trade_api_->SetSoftwareKey(params.auth_code().c_str());
  session_id_ = trade_api_->Login(ip, port, params.investor_id().c_str(),
                                  params.passwd().c_str(), sock_type);
  if (session_id_ == 0) {
    spdlog::error("[XtpTradeApi::login] Failed to login: {}",
                  trade_api_->GetApiLastError()->error_msg);
    return false;
  }

  return true;
}

void XtpTradeApi::logout() {
  if (session_id_ != 0) {
    trade_api_->Logout(session_id_);
    session_id_ = 0;
  }
}

bool XtpTradeApi::send_order(const OrderReq* order) {
  if (session_id_ == 0) {
    spdlog::error("[XtpTradeApi::send_order] Not logon");
    return false;
  }

  auto contract = ContractTable::get_by_index(order->ticker_index);
  if (!contract) {
    spdlog::error(
        "[XtpTradeApi::send_order] Contract not found. XtpOrderID: {}",
        order->order_id);
    return false;
  }

  XTPOrderInsertInfo req{};
  req.side = xtp_side(order->direction);
  if (req.side == XTP_SIDE_UNKNOWN) {
    spdlog::error("[XtpTradeApi::send_order] 不支持的交易类型");
    return false;
  }

  req.price_type = xtp_price_type(order->type);
  if (req.side == XTP_PRICE_TYPE::XTP_PRICE_TYPE_UNKNOWN) {
    spdlog::error("[XtpTradeApi::send_order] 不支持的订单价格类型");
    return false;
  }

  req.market = xtp_market_type(contract->exchange);
  if (req.market == XTP_MARKET_TYPE::XTP_MKT_UNKNOWN) {
    spdlog::error("[XtpTradeApi::send_order] Unknown exchange");
    return false;
  }

  req.order_client_id = next_client_order_id();
  strncpy(req.ticker, contract->symbol.c_str(), sizeof(req.ticker));
  req.price = order->price;
  req.quantity = order->volume;
  req.business_type = XTP_BUSINESS_TYPE::XTP_BUSINESS_TYPE_CASH;

  std::unique_lock<std::mutex> lock(order_mutex_);
  uint64_t xtp_order_id = trade_api_->InsertOrder(&req, session_id_);
  if (xtp_order_id == 0) {
    spdlog::error("[XtpTradeApi::send_order] 订单插入失败: {}",
                  trade_api_->GetApiLastError()->error_msg);
    return false;
  }

  OrderDetail detail{};
  detail.contract = contract;
  detail.order_id = order->order_id;
  detail.original_vol = order->volume;
  order_details_.emplace(xtp_order_id, detail);
  order_id_ft2xtp_.emplace(order->order_id, xtp_order_id);

  return true;
}

void XtpTradeApi::OnOrderEvent(XTPOrderInfo* order_info, XTPRI* error_info,
                               uint64_t session_id) {
  if (!order_info) {
    spdlog::warn("[XtpTradeApi::OnOrderEvent] nullptr");
    return;
  }

  std::unique_lock<std::mutex> lock(order_mutex_);
  auto iter = order_details_.find(order_info->order_xtp_id);
  if (iter == order_details_.end()) {
    spdlog::error("[XtpTradeApi::OnOrderEvent] Order not found. XtpOrderID: {}",
                  order_info->order_xtp_id);
    return;
  }
  auto& detail = iter->second;

  if (is_error_rsp(error_info)) {
    spdlog::error("[XtpTradeApi::OnOrderEvent] ErrorMsg: {}",
                  error_info->error_msg);
    engine_->on_order_rejected(detail.order_id);
    order_id_ft2xtp_.erase(detail.order_id);
    order_details_.erase(iter);
    return;
  }

  if (order_info->order_status ==
      XTP_ORDER_STATUS_TYPE::XTP_ORDER_STATUS_REJECTED) {
    engine_->on_order_rejected(detail.order_id);
    return;
  }

  if (order_info->order_status ==
      XTP_ORDER_STATUS_TYPE::XTP_ORDER_STATUS_UNKNOWN)
    return;

  if (!detail.accepted_ack) {
    engine_->on_order_accepted(detail.order_id);
    detail.accepted_ack = true;
  }

  if (order_info->order_status ==
          XTP_ORDER_STATUS_TYPE::XTP_ORDER_STATUS_CANCELED ||
      order_info->order_status ==
          XTP_ORDER_STATUS_TYPE::XTP_ORDER_STATUS_PARTTRADEDNOTQUEUEING) {
    if (detail.canceled_vol == 0) {
      detail.canceled_vol = order_info->qty_left;
      engine_->on_order_canceled(detail.order_id, detail.canceled_vol);

      if (detail.canceled_vol + detail.traded_vol == detail.original_vol) {
        order_id_ft2xtp_.erase(detail.order_id);
        order_details_.erase(iter);
      }
    }
  }
}

void XtpTradeApi::OnTradeEvent(XTPTradeReport* trade_info,
                               uint64_t session_id) {
  if (!trade_info) {
    spdlog::warn("[XtpTradeApi::OnTradeEvent] nullptr");
    return;
  }

  std::unique_lock<std::mutex> lock(order_mutex_);
  auto iter = order_details_.find(trade_info->order_xtp_id);
  if (iter == order_details_.end()) {
    spdlog::error("[XtpTradeApi::OnTradeEvent] Order not found. XtpOrderID: {}",
                  trade_info->order_xtp_id);
    return;
  }

  auto& detail = iter->second;
  detail.traded_vol += trade_info->trade_amount;
  engine_->on_order_traded(detail.order_id, trade_info->trade_amount,
                           trade_info->price);

  if (detail.traded_vol += detail.canceled_vol == detail.original_vol) {
    order_id_ft2xtp_.erase(detail.order_id);
    order_details_.erase(iter);
  }
}

bool XtpTradeApi::cancel_order(uint64_t order_id) {
  std::unique_lock<std::mutex> lock(order_mutex_);
  auto iter = order_id_ft2xtp_.find(order_id);
  if (iter == order_id_ft2xtp_.end()) {
    spdlog::error("[XtpTradeApi::cancel_order] Order not found. OrderID: {}",
                  order_id);
    return false;
  }

  uint64_t xtp_order_id = iter->second;
  auto& detail = order_details_.find(xtp_order_id)->second;
  if (!detail.accepted_ack) {
    spdlog::error("[XtpTradeApi::cancel_order] 未被交易所接受的订单不可撤");
    return false;
  }

  if (trade_api_->CancelOrder(xtp_order_id, session_id_) == 0) {
    spdlog::error("[XtpTradeApi::cancel_order] Failed to call CancelOrder");
    return false;
  }

  return true;
}

void XtpTradeApi::OnCancelOrderError(XTPOrderCancelInfo* cancel_info,
                                     XTPRI* error_info, uint64_t session_id) {
  if (!is_error_rsp(error_info)) return;

  if (!cancel_info) {
    spdlog::warn("[XtpTradeApi::OnCancelOrderError] nullptr");
    return;
  }

  std::unique_lock<std::mutex> lock(order_mutex_);
  auto iter = order_details_.find(cancel_info->order_xtp_id);
  if (iter == order_details_.end()) {
    spdlog::error(
        "[XtpTradeApi::OnCancelOrderError] Order not found. XtpOrderID: {}",
        cancel_info->order_xtp_id);
    return;
  }
  const auto& detail = iter->second;

  spdlog::error("[XtpTradeApi::OnCancelOrderError] Cancel error. ErrorMsg: {}",
                error_info->error_msg);
  engine_->on_order_cancel_rejected(detail.order_id);
}

bool XtpTradeApi::query_position(const std::string& ticker) {
  if (session_id_ == 0) return false;

  std::unique_lock<std::mutex> lock(query_mutex_);
  reset_sync();
  int res =
      trade_api_->QueryPosition(ticker.c_str(), session_id_, next_req_id());
  if (res != 0) {
    spdlog::error("[XtpTradeApi::query_position] Failed to call QueryPosition");
    return false;
  }
  return wait_sync();
}

bool XtpTradeApi::query_positions() { return query_position(""); }

void XtpTradeApi::OnQueryPosition(XTPQueryStkPositionRsp* position,
                                  XTPRI* error_info, int request_id,
                                  bool is_last, uint64_t session_id) {
  if (is_error_rsp(error_info)) {
    spdlog::error(
        "[CtpTradeApi::OnRspQryInvestorPosition] Failed. Error Msg: {}",
        error_info->error_msg);
    pos_cache_.clear();
    error();
    return;
  }

  if (position) {
    const auto* contract = ContractTable::get_by_symbol(position->ticker);
    if (!contract) {
      spdlog::error(
          "[CtpTradeApi::OnRspQryInvestorPosition] Contract not found");
      return;
    }

    auto& pos = pos_cache_[contract->index];
    pos.ticker_index = contract->index;

    bool is_long_pos =
        position->position_direction == XTP_POSITION_DIRECTION_LONG;
    auto& pos_detail = is_long_pos ? pos.long_pos : pos.short_pos;
    pos_detail.yd_volume = position->sellable_qty;
    pos_detail.volume = position->total_qty;
    pos_detail.float_pnl = position->unrealized_pnl;
    pos_detail.cost_price = position->avg_price;
  }

  if (is_last) {
    for (auto& [ticker_index, pos] : pos_cache_)
      engine_->on_query_position(&pos);
    pos_cache_.clear();
    done();
  }
}

bool XtpTradeApi::query_account() { return true; }

bool XtpTradeApi::query_orders() { return true; }

bool XtpTradeApi::query_trades() { return true; }

}  // namespace ft
