// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "gateway/xtp/xtp_trade_api.h"

#include <spdlog/spdlog.h>

#include <cstdlib>

#include "core/contract_table.h"
#include "utils/misc.h"

namespace ft {

XtpTradeApi::XtpTradeApi(TradingEngineInterface* engine) : engine_(engine) {}

XtpTradeApi::~XtpTradeApi() {
  error();
  logout();
}

bool XtpTradeApi::login(const Config& config) {
  if (session_id_ != 0) {
    spdlog::error("[XtpTradeApi::login] Don't login twice");
    return false;
  }

  investor_id_ = config.investor_id;

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
    int ret = sscanf(config.trade_server_address.c_str(), "%[^:]://%[^:]:%d",
                     protocol, ip, &port);
    assert(ret == 3);
  } catch (...) {
    assert(false);
  }

  XTP_PROTOCOL_TYPE sock_type = XTP_PROTOCOL_TCP;
  if (strcmp(protocol, "udp") == 0) sock_type = XTP_PROTOCOL_UDP;

  trade_api_->SubscribePublicTopic(XTP_TERT_QUICK);
  trade_api_->RegisterSpi(this);
  trade_api_->SetSoftwareKey(config.auth_code.c_str());
  session_id_ = trade_api_->Login(ip, port, config.investor_id.c_str(),
                                  config.password.c_str(), sock_type);
  if (session_id_ == 0) {
    spdlog::error("[XtpTradeApi::login] Failed to login: {}",
                  trade_api_->GetApiLastError()->error_msg);
    return false;
  }

  if (config.cancel_outstanding_orders_on_startup) {
    spdlog::debug("[XtpTradeApi::login] Cancel outstanding orders on startup");
    if (!query_orders()) {
      spdlog::error("[XtpTradeApi::login] 订单查询失败");
      return false;
    }
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
    spdlog::error("[XtpTradeApi::send_order] Contract not found");
    return false;
  }

  XTPOrderInsertInfo req{};
  req.side = xtp_side(order->direction);
  if (req.side == XTP_SIDE_UNKNOWN) {
    spdlog::error("[XtpTradeApi::send_order] 不支持的交易类型");
    return false;
  }

  req.price_type = xtp_price_type(order->type);
  if (req.side == XTP_PRICE_TYPE_UNKNOWN) {
    spdlog::error("[XtpTradeApi::send_order] 不支持的订单价格类型");
    return false;
  }

  req.market = xtp_market_type(contract->exchange);
  if (req.market == XTP_MKT_UNKNOWN) {
    spdlog::error("[XtpTradeApi::send_order] Unknown exchange");
    return false;
  }

  req.order_client_id = order->engine_order_id;
  strncpy(req.ticker, contract->ticker.c_str(), sizeof(req.ticker));
  req.price = order->price;
  req.quantity = order->volume;
  req.business_type = XTP_BUSINESS_TYPE_CASH;

  uint64_t xtp_order_id = trade_api_->InsertOrder(&req, session_id_);
  if (xtp_order_id == 0) {
    spdlog::error("[XtpTradeApi::send_order] 订单插入失败: {}",
                  trade_api_->GetApiLastError()->error_msg);
    return false;
  }

  spdlog::debug("[XtpTradeApi::send_order] 订单插入成功. XtpOrderID: {}",
                xtp_order_id);
  return true;
}

void XtpTradeApi::OnOrderEvent(XTPOrderInfo* order_info, XTPRI* error_info,
                               uint64_t session_id) {
  if (session_id != session_id_) return;

  if (!order_info) {
    spdlog::warn("[XtpTradeApi::OnOrderEvent] nullptr");
    return;
  }

  if (is_error_rsp(error_info)) {
    spdlog::error("[XtpTradeApi::OnOrderEvent] ErrorMsg: {}",
                  error_info->error_msg);
    return;
  }

  if (order_info->order_status == XTP_ORDER_STATUS_REJECTED) {
    engine_->on_order_rejected(order_info->order_client_id);
    return;
  }

  if (order_info->order_status == XTP_ORDER_STATUS_UNKNOWN) return;

  if (order_info->order_status == XTP_ORDER_STATUS_NOTRADEQUEUEING) {
    engine_->on_order_accepted(order_info->order_client_id,
                               order_info->order_xtp_id);
    return;
  }

  if (order_info->order_status == XTP_ORDER_STATUS_CANCELED ||
      order_info->order_status == XTP_ORDER_STATUS_PARTTRADEDNOTQUEUEING) {
    engine_->on_order_canceled(order_info->order_client_id,
                               order_info->qty_left);
  }
}

void XtpTradeApi::OnTradeEvent(XTPTradeReport* trade_info,
                               uint64_t session_id) {
  if (session_id_ != session_id) return;

  if (!trade_info) {
    spdlog::warn("[XtpTradeApi::OnTradeEvent] nullptr");
    return;
  }

  engine_->on_order_traded(trade_info->order_client_id,
                           trade_info->order_xtp_id, trade_info->quantity,
                           trade_info->price);
}

bool XtpTradeApi::cancel_order(uint64_t order_id) {
  if (trade_api_->CancelOrder(order_id, session_id_) == 0) {
    spdlog::error("[XtpTradeApi::cancel_order] Failed to call CancelOrder");
    return false;
  }

  return true;
}

void XtpTradeApi::OnCancelOrderError(XTPOrderCancelInfo* cancel_info,
                                     XTPRI* error_info, uint64_t session_id) {
  if (session_id_ != session_id) return;

  if (!is_error_rsp(error_info)) return;

  if (!cancel_info) {
    spdlog::warn("[XtpTradeApi::OnCancelOrderError] nullptr");
    return;
  }

  spdlog::error("[XtpTradeApi::OnCancelOrderError] Cancel error. ErrorMsg: {}",
                error_info->error_msg);
}

bool XtpTradeApi::query_position(const std::string& ticker) {
  if (session_id_ == 0) return false;

  std::unique_lock<std::mutex> lock(query_mutex_);
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
    spdlog::debug(
        "[XtpTradeApi::OnQueryPosition] Ticker: {}, TickerName: {}, YDPos: {}, "
        "Pos: {}, AvgPrice: {:.3f}, FloatPNL:{:.3f}",
        position->ticker, position->ticker_name, position->yesterday_position,
        position->total_qty, position->avg_price, position->unrealized_pnl);

    auto contract = ContractTable::get_by_ticker(position->ticker);
    if (!contract) {
      spdlog::error(
          "[CtpTradeApi::OnRspQryInvestorPosition] Contract not found. {}, {}",
          position->ticker, position->ticker_name);
      goto check_last;
    }

    auto& pos = pos_cache_[contract->index];
    pos.ticker_index = contract->index;

    // 暂时只支持普通股票
    auto& pos_detail = pos.long_pos;
    pos_detail.yd_holdings = position->sellable_qty;
    pos_detail.holdings = position->total_qty;
    pos_detail.float_pnl = position->unrealized_pnl;
    pos_detail.cost_price = position->avg_price;
  }

check_last:
  if (is_last) {
    for (auto& [ticker_index, pos] : pos_cache_) {
      UNUSED(ticker_index);
      engine_->on_query_position(&pos);
    }
    pos_cache_.clear();
    done();
  }
}

bool XtpTradeApi::query_account() {
  if (session_id_ == 0) return false;

  std::unique_lock<std::mutex> lock(query_mutex_);
  if (trade_api_->QueryAsset(session_id_, next_req_id()) != 0) {
    spdlog::error("[XtpTradeApi::query_account] {}",
                  trade_api_->GetApiLastError()->error_msg);
    return false;
  }

  return wait_sync();
}

void XtpTradeApi::OnQueryAsset(XTPQueryAssetRsp* asset, XTPRI* error_info,
                               int request_id, bool is_last,
                               uint64_t session_id) {
  UNUSED(request_id);

  if (session_id_ != session_id) return;

  if (is_error_rsp(error_info)) {
    spdlog::error("[XtpTradeApi::OnQueryAsset] {}", error_info->error_msg);
    error();
    return;
  }

  if (!asset) {
    spdlog::error("[[XtpTradeApi::OnQueryAsset] nullptr");
    error();
    return;
  }

  spdlog::debug("[XtpTradeApi::OnQueryAsset] TotalAsset: {}. Balance: {}",
                asset->total_asset, asset->banlance);
  Account account{};
  account.account_id = std::stoull(investor_id_);
  account.balance = asset->total_asset;
  engine_->on_query_account(&account);

  if (is_last) done();
}

bool XtpTradeApi::query_orders() {
  if (session_id_ == 0) return false;

  XTPQueryOrderReq req{};

  std::unique_lock<std::mutex> lock(query_mutex_);
  if (trade_api_->QueryOrders(&req, session_id_, next_req_id()) != 0) {
    spdlog::error("[XtpTradeApi::query_orders] {}",
                  trade_api_->GetApiLastError()->error_msg);
    return false;
  }

  return wait_sync();
}

void XtpTradeApi::OnQueryOrder(XTPQueryOrderRsp* order_info, XTPRI* error_info,
                               int request_id, bool is_last,
                               uint64_t session_id) {
  UNUSED(request_id);

  if (session_id_ != session_id) return;

  if (is_error_rsp(error_info)) {
    spdlog::error("[XtpTradeApi::OnQueryOrder] {}", error_info->error_msg);
    done();
    return;
  }

  if (order_info &&
      (order_info->order_status == XTP_ORDER_STATUS_NOTRADEQUEUEING ||
       order_info->order_status == XTP_ORDER_STATUS_PARTTRADEDQUEUEING)) {
    if (trade_api_->CancelOrder(order_info->order_xtp_id, session_id_) == 0)
      spdlog::error("[XtpTradeApi::OnQueryOrder] 订单撤回失败: {}",
                    trade_api_->GetApiLastError()->error_msg);
  }

  if (is_last) done();
}

bool XtpTradeApi::query_trades() {
  if (session_id_ == 0) return false;

  XTPQueryTraderReq req{};

  std::unique_lock<std::mutex> lock(query_mutex_);
  if (trade_api_->QueryTrades(&req, session_id_, next_req_id()) != 0) {
    spdlog::error("[XtpTradeApi::query_trades] {}",
                  trade_api_->GetApiLastError()->error_msg);
    return false;
  }

  return wait_sync();
}

void XtpTradeApi::OnQueryTrade(XTPQueryTradeRsp* trade_info, XTPRI* error_info,
                               int request_id, bool is_last,
                               uint64_t session_id) {
  UNUSED(request_id);

  if (session_id_ != session_id) return;

  if (is_error_rsp(error_info)) {
    spdlog::error("[XtpTradeApi::OnQueryTrade] {}", error_info->error_msg);
    done();
    return;
  }

  if (trade_info) {
    auto contract = ContractTable::get_by_ticker(trade_info->ticker);
    assert(contract);

    Trade trade{};
    trade.ticker_index = contract->index;
    trade.volume = trade_info->quantity;
    trade.price = trade_info->price;
    if (trade_info->side == XTP_SIDE_BUY) {
      trade.direction = Direction::BUY;
      trade.offset = Offset::OPEN;
    } else if (trade_info->side == XTP_SIDE_SELL) {
      trade.direction = Direction::SELL;
      trade.offset = Offset::CLOSE_YESTERDAY;
    } else {
      assert(false);
    }
    engine_->on_query_trade(&trade);
  }

  if (is_last) done();
}

}  // namespace ft
