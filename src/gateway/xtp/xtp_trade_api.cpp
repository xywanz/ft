// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "gateway/xtp/xtp_trade_api.h"

#include <algorithm>
#include <cstdlib>

#include "ft/base/contract_table.h"
#include "ft/utils/misc.h"
#include "ft/utils/protocol_utils.h"
#include "spdlog/spdlog.h"

namespace ft {

XtpTradeApi::XtpTradeApi(BaseOrderManagementSystem* oms) : oms_(oms) {
  uint32_t seed = time(nullptr);
  uint8_t client_id = rand_r(&seed) & 0xff;
  trade_api_.reset(XTP::API::TraderApi::CreateTraderApi(client_id, "."));
  if (!trade_api_) {
    spdlog::error("[XtpTradeApi::XtpTradeApi] Failed to CreateTraderApi");
    exit(-1);
  }
}

XtpTradeApi::~XtpTradeApi() {
  error();
  Logout();
}

bool XtpTradeApi::Login(const Config& config) {
  investor_id_ = config.investor_id;

  char protocol[32]{};
  char ip[32]{};
  int port = 0;

  try {
    int ret = sscanf(config.trade_server_address.c_str(), "%[^:]://%[^:]:%d", protocol, ip, &port);
    if (ret != 3) {
      spdlog::error("[XtpTradeApi::Login] Invalid trade server: {}", config.trade_server_address);
      return false;
    }
  } catch (...) {
    spdlog::error("[XtpTradeApi::Login] Invalid trade server: {}", config.trade_server_address);
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
    spdlog::error("[XtpTradeApi::Login] Failed to Call API Login: {}",
                  trade_api_->GetApiLastError()->error_msg);
    return false;
  }

  if (config.cancel_outstanding_orders_on_startup) {
    spdlog::debug("[XtpTradeApi::Login] Cancel outstanding orders on startup");
    if (!QueryOrderList()) {
      spdlog::error(
          "[XtpTradeApi::Login] Failed to query orders and cancel outstanding "
          "orders");
      return false;
    }
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
    spdlog::info("[XtpTradeApi::SendOrder] 不支持BuyClose或是SellOpen");
    return false;
  }

  auto contract = order.contract;

  XTPOrderInsertInfo req{};
  req.side = xtp_side(order.direction);
  if (req.side == XTP_SIDE_UNKNOWN) {
    spdlog::error("[XtpTradeApi::SendOrder] 不支持的交易类型");
    return false;
  }

  if (order.direction == Direction::kBuy || order.direction == Direction::kSell) {
    req.price_type = xtp_price_type(order.type);
    if (req.side == XTP_PRICE_TYPE_UNKNOWN) {
      spdlog::error("[XtpTradeApi::SendOrder] 不支持的订单价格类型");
      return false;
    }
    req.business_type = XTP_BUSINESS_TYPE_CASH;
    req.price = order.price;
  } else {
    req.price_type = XTP_PRICE_LIMIT;
    req.business_type = XTP_BUSINESS_TYPE_ETF;
  }

  req.market = xtp_market_type(contract->exchange);
  if (req.market == XTP_MKT_UNKNOWN) {
    spdlog::error("[XtpTradeApi::SendOrder] Unknown exchange");
    return false;
  }

  req.order_client_id = order.order_id;
  strncpy(req.ticker, contract->ticker.c_str(), sizeof(req.ticker));
  req.quantity = order.volume;

  uint64_t xtp_order_id = trade_api_->InsertOrder(&req, session_id_);
  if (xtp_order_id == 0) {
    spdlog::error("[XtpTradeApi::SendOrder] 订单插入失败: {}",
                  trade_api_->GetApiLastError()->error_msg);
    return false;
  }

  spdlog::debug("[XtpTradeApi::SendOrder] 订单插入成功. XtpOrderID: {}", xtp_order_id);
  *privdata_ptr = xtp_order_id;
  return true;
}

void XtpTradeApi::OnOrderEvent(XTPOrderInfo* order_info, XTPRI* error_info, uint64_t session_id) {
  if (session_id != session_id_) return;

  if (!order_info) {
    spdlog::warn("[XtpTradeApi::OnOrderEvent] nullptr");
    return;
  }

  if (is_error_rsp(error_info)) {
    OrderRejection rsp = {order_info->order_client_id, error_info->error_msg};
    oms_->OnOrderRejected(&rsp);
    return;
  }

  if (order_info->order_status == XTP_ORDER_STATUS_REJECTED) {
    OrderRejection rsp = {order_info->order_client_id, error_info->error_msg};
    oms_->OnOrderRejected(&rsp);
    return;
  }

  if (order_info->order_status == XTP_ORDER_STATUS_UNKNOWN) return;

  if (order_info->order_status == XTP_ORDER_STATUS_NOTRADEQUEUEING) {
    OrderAcceptance rsp = {order_info->order_client_id};
    oms_->OnOrderAccepted(&rsp);
    return;
  }

  if (order_info->order_status == XTP_ORDER_STATUS_CANCELED ||
      order_info->order_status == XTP_ORDER_STATUS_PARTTRADEDNOTQUEUEING) {
    OrderCancellation rsp = {order_info->order_client_id, static_cast<int>(order_info->qty_left)};
    oms_->OnOrderCanceled(&rsp);
  }
}

void XtpTradeApi::OnTradeEvent(XTPTradeReport* trade_info, uint64_t session_id) {
  if (session_id_ != session_id) return;

  if (!trade_info) {
    spdlog::warn("[XtpTradeApi::OnTradeEvent] nullptr");
    return;
  }

  spdlog::trace("ETF purchase/redeem: {},{},{},{},{},{},{}", trade_info->ticker,
                trade_info->business_type, trade_info->trade_type, trade_info->side,
                trade_info->quantity, trade_info->price, trade_info->trade_amount);

  Trade rsp{};
  if (trade_info->business_type == XTP_BUSINESS_TYPE_ETF) {
    auto contract = ContractTable::get_by_ticker(trade_info->ticker);
    if (!contract) {
      trade_info->ticker[5] = '0';
      contract = ContractTable::get_by_ticker(trade_info->ticker);
      if (!contract) {
        spdlog::warn("[XtpTradeApi::OnTradeEvent] Contract not found: {}", trade_info->ticker);
        return;
      }
    }

    // 申赎时第一个返回的回报可能是这个类型的
    // 即ETF的普通成交类型
    // 这个信息对我们来说没有用处，反而会扰乱仓位的计算
    // 所以在此处过滤掉该回报
    if (contract->product_type == ProductType::kFund && trade_info->trade_type == XTP_TRDT_COMMON)
      return;

    // 收到成分股回报，需要设置tid, direction等信息
    rsp.ticker_id = contract->ticker_id;
    rsp.direction =
        trade_info->side == XTP_SIDE_PURCHASE ? Direction::kPurchase : Direction::kRedeem;
    rsp.trade_type = ft_trade_type(trade_info->side, trade_info->trade_type);
    rsp.amount = trade_info->trade_amount;
  } else {
    // 二级市场成交，这里无需设置tid，因为该tid和order_req中
    // 的是一样的；direction和offset同理
    rsp.trade_type = TradeType::kSecondaryMarket;
  }

  rsp.order_id = trade_info->order_client_id;
  rsp.volume = trade_info->quantity;
  rsp.price = trade_info->price;
  rsp.trade_time =
      datetime::strptime(fmt::format("{:017}", trade_info->trade_time), "%Y%m%d%H%M%S%s");
  oms_->OnOrderTraded(&rsp);
}

bool XtpTradeApi::CancelOrder(uint64_t xtp_order_id) {
  if (trade_api_->CancelOrder(xtp_order_id, session_id_) == 0) {
    spdlog::error("[XtpTradeApi::CancelOrder] Failed to call CancelOrder");
    return false;
  }

  return true;
}

void XtpTradeApi::OnCancelOrderError(XTPOrderCancelInfo* cancel_info, XTPRI* error_info,
                                     uint64_t session_id) {
  if (session_id_ != session_id) return;

  if (!is_error_rsp(error_info)) return;

  if (!cancel_info) {
    spdlog::warn("[XtpTradeApi::OnCancelOrderError] nullptr");
    return;
  }

  spdlog::error("[XtpTradeApi::OnCancelOrderError] Cancel error. ErrorMsg: {}",
                error_info->error_msg);
}

bool XtpTradeApi::QueryPositionList(std::vector<Position>* result) {
  position_results_ = result;
  int res = trade_api_->QueryPosition("", session_id_, next_req_id());
  if (res != 0) {
    spdlog::error("[XtpTradeApi::QueryPosition] Failed to call QueryPosition");
    return false;
  }
  return wait_sync();
}

void XtpTradeApi::OnQueryPosition(XTPQueryStkPositionRsp* position, XTPRI* error_info,
                                  int request_id, bool is_last, uint64_t session_id) {
  if (is_error_rsp(error_info)) {
    spdlog::error("[CtpTradeApi::OnRspQryInvestorPosition] Failed. Error Msg: {}",
                  error_info->error_msg);
    pos_cache_.clear();
    error();
    return;
  }

  if (position) {
    spdlog::debug(
        "[XtpTradeApi::OnQueryPosition] Ticker: {}, TickerName: {}, YDPos: {}, "
        "Pos: {}, AvgPrice: {:.3f}, FloatPNL:{:.3f}",
        position->ticker, position->ticker_name, position->yesterday_position, position->total_qty,
        position->avg_price, position->unrealized_pnl);

    auto contract = ContractTable::get_by_ticker(position->ticker);
    if (!contract) {
      spdlog::error("[CtpTradeApi::OnRspQryInvestorPosition] Contract not found. {}, {}",
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
      UNUSED(ticker_id);
      if (position_results_) position_results_->emplace_back(pos);
    }
    pos_cache_.clear();
    done();
  }
}

bool XtpTradeApi::QueryAccount(Account* result) {
  account_result_ = result;
  if (trade_api_->QueryAsset(session_id_, next_req_id()) != 0) {
    spdlog::error("[XtpTradeApi::QueryAccount] {}", trade_api_->GetApiLastError()->error_msg);
    return false;
  }

  return wait_sync();
}

void XtpTradeApi::OnQueryAsset(XTPQueryAssetRsp* asset, XTPRI* error_info, int request_id,
                               bool is_last, uint64_t session_id) {
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

  spdlog::debug(
      "[XtpTradeApi::OnQueryAsset] total_asset:{}, buying_power:{}, "
      "security_asset:{}, fund_buy_amount:{}, fund_buy_fee:{}, "
      "fund_sell_amount:{}, fund_sell_fee:{}, withholding_amount:{}, "
      "account_type:{}, frozen_margin:{}, frozen_exec_cash:{}, "
      "frozen_exec_fee:{}, pay_later:{}, preadva_pay:{}, orig_banlance:{}, "
      "banlance:{}, deposit_withdraw:{}, trade_netting:{}, captial_asset:{}, "
      "force_freeze_amount:{}, preferred_amount:{}, "
      "repay_stock_aval_banlance:{}",
      asset->total_asset, asset->buying_power, asset->security_asset, asset->fund_buy_amount,
      asset->fund_buy_fee, asset->fund_sell_amount, asset->fund_sell_fee, asset->withholding_amount,
      asset->account_type, asset->frozen_margin, asset->frozen_exec_cash, asset->frozen_exec_fee,
      asset->pay_later, asset->preadva_pay, asset->orig_banlance, asset->banlance,
      asset->deposit_withdraw, asset->trade_netting, asset->captial_asset,
      asset->force_freeze_amount, asset->preferred_amount, asset->repay_stock_aval_banlance);
  Account account{};
  account.account_id = std::stoull(investor_id_);
  account.total_asset = asset->total_asset;
  account.cash = asset->buying_power;
  account.margin = 0;
  account.frozen = asset->withholding_amount;

  if (account_result_) *account_result_ = account;
  if (is_last) done();
}

bool XtpTradeApi::QueryOrderList() {
  XTPQueryOrderReq req{};

  if (trade_api_->QueryOrders(&req, session_id_, next_req_id()) != 0) {
    spdlog::error("[XtpTradeApi::QueryOrderList] {}", trade_api_->GetApiLastError()->error_msg);
    return false;
  }

  return wait_sync();
}

void XtpTradeApi::OnQueryOrder(XTPQueryOrderRsp* order_info, XTPRI* error_info, int request_id,
                               bool is_last, uint64_t session_id) {
  UNUSED(request_id);

  if (session_id_ != session_id) return;

  if (is_error_rsp(error_info)) {
    spdlog::error("[XtpTradeApi::OnQueryOrder] {}", error_info->error_msg);
    done();
    return;
  }

  if (order_info && (order_info->order_status == XTP_ORDER_STATUS_NOTRADEQUEUEING ||
                     order_info->order_status == XTP_ORDER_STATUS_PARTTRADEDQUEUEING)) {
    if (trade_api_->CancelOrder(order_info->order_xtp_id, session_id_) == 0)
      spdlog::error("[XtpTradeApi::OnQueryOrder] 订单撤回失败: {}",
                    trade_api_->GetApiLastError()->error_msg);
  }

  if (is_last) done();
}

bool XtpTradeApi::QueryTradeList(std::vector<Trade>* result) {
  trade_results_ = result;

  XTPQueryTraderReq req{};
  if (trade_api_->QueryTrades(&req, session_id_, next_req_id()) != 0) {
    spdlog::error("[XtpTradeApi::QueryTradeList] {}", trade_api_->GetApiLastError()->error_msg);
    return false;
  }

  return wait_sync();
}

void XtpTradeApi::OnQueryTrade(XTPQueryTradeRsp* trade_info, XTPRI* error_info, int request_id,
                               bool is_last, uint64_t session_id) {
  UNUSED(request_id);

  if (session_id_ != session_id) return;

  if (is_error_rsp(error_info)) {
    spdlog::error("[XtpTradeApi::OnQueryTrade] {}", error_info->error_msg);
    done();
    return;
  }

  if (trade_info && (trade_info->side == XTP_SIDE_BUY || trade_info->side == XTP_SIDE_SELL)) {
    auto contract = ContractTable::get_by_ticker(trade_info->ticker);
    assert(contract);

    Trade trade{};
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

    if (trade_results_) trade_results_->emplace_back(trade);
  }

  if (is_last) done();
}

}  // namespace ft
