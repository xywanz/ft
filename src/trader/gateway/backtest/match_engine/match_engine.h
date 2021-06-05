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

  virtual void OnAccepted(const OrderRequest& order) {}

  virtual void OnRejected(const OrderRequest& order) {}

  virtual void OnTraded(const OrderRequest& order, int volume, double price,
                        uint64_t timestamp_us) {}

  virtual void OnCanceled(const OrderRequest& order, int canceled_volume) {}

  virtual void OnCancelRejected(uint64_t order_id) {}
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
