// Copyright [2020-present] <Copyright Kevin, kevin.lau.gd@gmail.com>

#pragma once

#include <map>
#include <memory>
#include <string>

#include "ft/base/market_data.h"
#include "trader/msg.h"

namespace ft {

class OrderEventListener {
 public:
  virtual ~OrderEventListener() {}

  virtual void OnAccepted(OrderAcceptedRsp* rsp) {}

  virtual void OnRejected(OrderRejectedRsp* rsp) {}

  virtual void OnTraded(OrderTradedRsp* rsp) {}

  virtual void OnCanceled(OrderCanceledRsp* rsp) {}

  virtual void OnCancelRejected(OrderCancelRejectedRsp* rsp) {}
};

class MatchEngine {
 public:
  void RegisterListener(OrderEventListener* listener) { listener_ = listener; }

  virtual bool Init() = 0;

  virtual bool InsertOrder(const OrderRequest& order) = 0;

  virtual bool CancelOrder(uint64_t order_id, uint32_t ticker_id) = 0;

  virtual void OnNewTick(const TickData& tick) = 0;

  OrderEventListener* listener() { return listener_; }

 private:
  OrderEventListener* listener_ = nullptr;
};

std::shared_ptr<MatchEngine> CreateMatchEngine(const std::string& name);

}  // namespace ft
