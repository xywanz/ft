// Copyright [2020-present] <Copyright Kevin, kevin.lau.gd@gmail.com>

#pragma once

#include <map>
#include <memory>
#include <string>

#include "ft/base/market_data.h"

namespace ft {

class MarketDataListener {
 public:
  virtual ~MarketDataListener() {}

  virtual void OnDataFeed(TickData* tick) = 0;
};

class DataFeed {
 public:
  void RegisterListener(MarketDataListener* listener) { listener_ = listener; }

  virtual bool Init(const std::map<std::string, std::string>& args) = 0;

  virtual bool Feed() = 0;

 protected:
  MarketDataListener* listener() { return listener_; }

 private:
  MarketDataListener* listener_ = nullptr;
};

std::shared_ptr<DataFeed> CreateDataFeed(const std::string& name);

}  // namespace ft
