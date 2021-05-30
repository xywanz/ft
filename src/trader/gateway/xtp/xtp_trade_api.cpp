// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "trader/gateway/xtp/xtp_trade_api.h"

#include <algorithm>
#include <cstdlib>
#include <thread>

#include "ft/base/contract_table.h"
#include "ft/base/log.h"
#include "ft/utils/misc.h"
#include "ft/utils/protocol_utils.h"
#include "trader/gateway/xtp/xtp_gateway.h"

namespace ft {

XtpTradeApi::XtpTradeApi(XtpGateway* gateway) : gateway_(gateway) {
  uint32_t seed = time(nullptr);
  uint8_t client_id = rand_r(&seed) % 99 + 1;
  trade_api_.reset(XTP::API::TraderApi::CreateTraderApi(client_id, "."));
  if (!trade_api_) {
    throw std::runtime_error("failed to create xtp trader api");
  }
}

XtpTradeApi::~XtpTradeApi() { Logout(); }

bool XtpTradeApi::Login(const GatewayConfig& config) {
  investor_id_ = config.investor_id;

  char protocol[32]{};
  char ip[32]{};
  int port = 0;

  try {
    int ret = sscanf(config.trade_server_address.c_str(), "%[^:]://%[^:]:%d", protocol, ip, &port);
    if (ret != 3) {
      LOG_ERROR("[XtpTradeApi::Login] Invalid trade server: {}", config.trade_server_address);
      return false;
    }
  } catch (...) {
    LOG_ERROR("[XtpTradeApi::Login] Invalid trade server: {}", config.trade_server_address);
    return false;
  }

  XTP_PROTOCOL_TYPE sock_type = XTP_PROTOCOL_TCP;
  if (strcmp(protocol, "udp") == 0) sock_type = XTP_PROTOCOL_UDP;

  trade_api_->SubscribePublicTopic(XTP_TERT_QUICK);
  trade_api_->RegisterSpi(this);
  trade_api_->SetSoftwareKey(config.auth_code.c_str());
  session_id_ =
      trade_api_->Login(ip, port, config.investor_id.c_str(), config.password.c_str(), sock_type);
  if (session_id_ == 0) {
    LOG_ERROR("[XtpTradeApi::Login] Failed to Call API Login: {}",
              trade_api_->GetApiLastError()->error_msg);
    return false;
  }

  if (config.cancel_outstanding_orders_on_startup) {
    LOG_DEBUG("[XtpTradeApi::Login] Cancel outstanding orders on startup");
    if (!QueryOrders()) {
      LOG_ERROR(
          "[XtpTradeApi::Login] Failed to query orders and cancel outstanding "
          "orders");
      return false;
    }

    while (status_.load(std::memory_order::memory_order_relaxed) == 0) {
      continue;
    }
    return status_.load(std::memory_order::memory_order_acquire) == 1;
  }

  return true;
}

void XtpTradeApi::Logout() {
  if (session_id_ != 0) {
    trade_api_->Logout(session_id_);
    session_id_ = 0;
  }
}

bool XtpTradeApi::SendOrder(const OrderRequest& order, uint64_t* privdata_ptr) {
  if ((order.direction == Direction::kBuy && IsOffsetClose(order.offset)) ||
      (order.direction == Direction::kSell && IsOffsetOpen(order.offset))) {
    LOG_ERROR("[XtpTradeApi::SendOrder] 不支持BuyClose或是SellOpen");
    return false;
  }

  auto contract = order.contract;

  XTPOrderInsertInfo req{};
  req.side = xtp_side(order.direction);
  if (req.side == XTP_SIDE_UNKNOWN) {
    LOG_ERROR("[XtpTradeApi::SendOrder] 不支持的交易类型");
    return false;
  }

  req.price_type = xtp_price_type(order.type);
  if (req.side == XTP_PRICE_TYPE_UNKNOWN) {
    LOG_ERROR("[XtpTradeApi::SendOrder] 不支持的订单价格类型");
    return false;
  }
  req.business_type = XTP_BUSINESS_TYPE_CASH;
  req.price = order.price;

  req.market = xtp_market_type(contract->exchange);
  if (req.market == XTP_MKT_UNKNOWN) {
    LOG_ERROR("[XtpTradeApi::SendOrder] Unknown exchange");
    return false;
  }

  req.order_client_id = order.order_id;
  strncpy(req.ticker, contract->ticker.c_str(), sizeof(req.ticker));
  req.quantity = order.volume;

  uint64_t xtp_order_id = trade_api_->InsertOrder(&req, session_id_);
  if (xtp_order_id == 0) {
    LOG_ERROR("[XtpTradeApi::SendOrder] 订单插入失败: {}",
              trade_api_->GetApiLastError()->error_msg);
    return false;
  }

  LOG_DEBUG("[XtpTradeApi::SendOrder] 订单插入成功. XtpOrderID: {}", xtp_order_id);
  *privdata_ptr = xtp_order_id;
  return true;
}

void XtpTradeApi::OnOrderEvent(XTPOrderInfo* order_info, XTPRI* error_info, uint64_t session_id) {
  if (session_id != session_id_) return;

  if (!order_info) {
    LOG_WARN("[XtpTradeApi::OnOrderEvent] nullptr");
    return;
  }

  if (is_error_rsp(error_info)) {
    OrderRejectedRsp rsp = {order_info->order_client_id, error_info->error_msg};
    gateway_->OnOrderRejected(rsp);
    return;
  }

  if (order_info->order_status == XTP_ORDER_STATUS_REJECTED) {
    OrderRejectedRsp rsp = {order_info->order_client_id, error_info->error_msg};
    gateway_->OnOrderRejected(rsp);
    return;
  }

  if (order_info->order_status == XTP_ORDER_STATUS_UNKNOWN) return;

  if (order_info->order_status == XTP_ORDER_STATUS_NOTRADEQUEUEING) {
    OrderAcceptedRsp rsp = {order_info->order_client_id};
    gateway_->OnOrderAccepted(rsp);
    return;
  }

  if (order_info->order_status == XTP_ORDER_STATUS_CANCELED ||
      order_info->order_status == XTP_ORDER_STATUS_PARTTRADEDNOTQUEUEING) {
    OrderCanceledRsp rsp = {order_info->order_client_id, static_cast<int>(order_info->qty_left)};
    gateway_->OnOrderCanceled(rsp);
  }
}

void XtpTradeApi::OnTradeEvent(XTPTradeReport* trade_info, uint64_t session_id) {
  if (session_id_ != session_id) return;

  if (!trade_info) {
    LOG_WARN("[XtpTradeApi::OnTradeEvent] nullptr");
    return;
  }

  dt_converter_.UpdateDate(trade_info->trade_time);

  OrderTradedRsp rsp{};
  rsp.order_id = trade_info->order_client_id;
  rsp.volume = trade_info->quantity;
  rsp.price = trade_info->price;
  rsp.timestamp_us = dt_converter_.GetExchTimeStamp(trade_info->trade_time);
  gateway_->OnOrderTraded(rsp);
}

bool XtpTradeApi::CancelOrder(uint64_t xtp_order_id) {
  if (trade_api_->CancelOrder(xtp_order_id, session_id_) == 0) {
    LOG_ERROR("[XtpTradeApi::CancelOrder] Failed to call CancelOrder");
    return false;
  }

  return true;
}

void XtpTradeApi::OnCancelOrderError(XTPOrderCancelInfo* cancel_info, XTPRI* error_info,
                                     uint64_t session_id) {
  if (session_id_ != session_id) return;

  if (!is_error_rsp(error_info)) return;

  if (!cancel_info) {
    LOG_WARN("[XtpTradeApi::OnCancelOrderError] nullptr");
    return;
  }

  LOG_ERROR("[XtpTradeApi::OnCancelOrderError] Cancel error. ErrorMsg: {}", error_info->error_msg);
}

bool XtpTradeApi::QueryPositions() {
  int res = trade_api_->QueryPosition("", session_id_, next_req_id());
  if (res != 0) {
    LOG_ERROR("[XtpTradeApi::QueryPosition] Failed to call QueryPosition");
    return false;
  }
  return true;
}

void XtpTradeApi::OnQueryPosition(XTPQueryStkPositionRsp* position, XTPRI* error_info,
                                  int request_id, bool is_last, uint64_t session_id) {
  if (is_error_rsp(error_info)) {
    LOG_ERROR("[CtpTradeApi::OnRspQryInvestorPosition] Failed. Error Msg: {}",
              error_info->error_msg);
    pos_cache_.clear();
    gateway_->OnQueryPositionEnd();
    return;
  }

  if (position) {
    LOG_DEBUG(
        "[XtpTradeApi::OnQueryPosition] Ticker: {}, TickerName: {}, YDPos: {}, "
        "Pos: {}, AvgPrice: {:.3f}, FloatPNL:{:.3f}",
        position->ticker, position->ticker_name, position->yesterday_position, position->total_qty,
        position->avg_price, position->unrealized_pnl);

    auto contract = ContractTable::get_by_ticker(position->ticker);
    if (!contract) {
      LOG_ERROR("[CtpTradeApi::OnRspQryInvestorPosition] Contract not found. {}, {}",
                position->ticker, position->ticker_name);
      goto check_last;
    }

    auto& pos = pos_cache_[contract->ticker_id];
    pos.ticker_id = contract->ticker_id;

    // 暂时只支持普通股票
    auto& pos_detail = pos.long_pos;
    pos_detail.yd_holdings = position->sellable_qty;
    pos_detail.holdings = std::max(position->sellable_qty, position->total_qty);
    pos_detail.float_pnl = position->unrealized_pnl;
    pos_detail.cost_price = position->avg_price;
  }

check_last:
  if (is_last) {
    for (auto& [ticker_id, pos] : pos_cache_) {
      gateway_->OnQueryPosition(pos);
    }
    gateway_->OnQueryPositionEnd();
    pos_cache_.clear();
  }
}

bool XtpTradeApi::QueryAccount() {
  if (trade_api_->QueryAsset(session_id_, next_req_id()) != 0) {
    LOG_ERROR("[XtpTradeApi::QueryAccount] {}", trade_api_->GetApiLastError()->error_msg);
    return false;
  }
  return true;
}

void XtpTradeApi::OnQueryAsset(XTPQueryAssetRsp* asset, XTPRI* error_info, int request_id,
                               bool is_last, uint64_t session_id) {
  if (session_id_ != session_id) return;

  if (is_error_rsp(error_info)) {
    LOG_ERROR("[XtpTradeApi::OnQueryAsset] {}", error_info->error_msg);
    gateway_->OnQueryAccountEnd();
    return;
  }

  if (asset) {
    LOG_DEBUG(
        "[XtpTradeApi::OnQueryAsset] total_asset:{}, buying_power:{}, "
        "security_asset:{}, fund_buy_amount:{}, fund_buy_fee:{}, "
        "fund_sell_amount:{}, fund_sell_fee:{}, withholding_amount:{}, "
        "account_type:{}, frozen_margin:{}, frozen_exec_cash:{}, "
        "frozen_exec_fee:{}, pay_later:{}, preadva_pay:{}, orig_banlance:{}, "
        "banlance:{}, deposit_withdraw:{}, trade_netting:{}, captial_asset:{}, "
        "force_freeze_amount:{}, preferred_amount:{}, "
        "repay_stock_aval_banlance:{}",
        asset->total_asset, asset->buying_power, asset->security_asset, asset->fund_buy_amount,
        asset->fund_buy_fee, asset->fund_sell_amount, asset->fund_sell_fee,
        asset->withholding_amount, asset->account_type, asset->frozen_margin,
        asset->frozen_exec_cash, asset->frozen_exec_fee, asset->pay_later, asset->preadva_pay,
        asset->orig_banlance, asset->banlance, asset->deposit_withdraw, asset->trade_netting,
        asset->captial_asset, asset->force_freeze_amount, asset->preferred_amount,
        asset->repay_stock_aval_banlance);
    Account account{};
    account.account_id = std::stoull(investor_id_);
    account.total_asset = asset->total_asset;
    account.cash = asset->buying_power;
    account.margin = 0;
    account.frozen = asset->withholding_amount;
    gateway_->OnQueryAccount(account);
  }

  if (is_last) {
    gateway_->OnQueryAccountEnd();
  }
}

bool XtpTradeApi::QueryOrders() {
  XTPQueryOrderReq req{};

  if (trade_api_->QueryOrders(&req, session_id_, next_req_id()) != 0) {
    LOG_ERROR("[XtpTradeApi::QueryOrderList] {}", trade_api_->GetApiLastError()->error_msg);
    return false;
  }
  return true;
}

void XtpTradeApi::OnQueryOrder(XTPQueryOrderRsp* order_info, XTPRI* error_info, int request_id,
                               bool is_last, uint64_t session_id) {
  UNUSED(request_id);

  if (session_id_ != session_id) return;

  if (is_error_rsp(error_info)) {
    LOG_ERROR("[XtpTradeApi::OnQueryOrder] {}", error_info->error_msg);
    status_.store(1, std::memory_order::memory_order_release);
    return;
  }

  if (order_info && (order_info->order_status == XTP_ORDER_STATUS_NOTRADEQUEUEING ||
                     order_info->order_status == XTP_ORDER_STATUS_PARTTRADEDQUEUEING)) {
    if (trade_api_->CancelOrder(order_info->order_xtp_id, session_id_) == 0)
      LOG_ERROR("[XtpTradeApi::OnQueryOrder] 订单撤回失败: {}",
                trade_api_->GetApiLastError()->error_msg);
  }

  if (is_last) {
    status_.store(1, std::memory_order::memory_order_release);
  }
}

bool XtpTradeApi::QueryTrades() {
  XTPQueryTraderReq req{};
  if (trade_api_->QueryTrades(&req, session_id_, next_req_id()) != 0) {
    LOG_ERROR("[XtpTradeApi::QueryTradeList] {}", trade_api_->GetApiLastError()->error_msg);
    return false;
  }
  return true;
}

void XtpTradeApi::OnQueryTrade(XTPQueryTradeRsp* trade_info, XTPRI* error_info, int request_id,
                               bool is_last, uint64_t session_id) {
  UNUSED(request_id);

  if (session_id_ != session_id) return;

  if (is_error_rsp(error_info)) {
    LOG_ERROR("[XtpTradeApi::OnQueryTrade] {}", error_info->error_msg);
    gateway_->OnQueryTradeEnd();
    return;
  }

  if (trade_info && (trade_info->side == XTP_SIDE_BUY || trade_info->side == XTP_SIDE_SELL)) {
    auto contract = ContractTable::get_by_ticker(trade_info->ticker);
    assert(contract);

    HistoricalTrade trade{};
    trade.order_sys_id = trade_info->order_xtp_id;
    trade.ticker_id = contract->ticker_id;
    trade.volume = trade_info->quantity;
    trade.price = trade_info->price;
    if (trade_info->side == XTP_SIDE_BUY) {
      trade.direction = Direction::kBuy;
      trade.offset = Offset::kOpen;
    } else if (trade_info->side == XTP_SIDE_SELL) {
      trade.direction = Direction::kSell;
      trade.offset = Offset::kCloseYesterday;
    }

    gateway_->OnQueryTrade(trade);
  }

  if (is_last) {
    gateway_->OnQueryTradeEnd();
  }
}

}  // namespace ft
