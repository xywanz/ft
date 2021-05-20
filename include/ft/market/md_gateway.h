// Copyright [2020-2021] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_INCLUDE_FT_MARKET_MD_GATEWAY_H_
#define FT_INCLUDE_FT_MARKET_MD_GATEWAY_H_

#include <string>
#include <vector>

#include "ft/base/config.h"
#include "ft/base/market_data.h"
#include "ft/utils/ring_buffer.h"

namespace ft {

class MdGateway {
 private:
  using TickRB = RingBuffer<TickData, 4096 * 16>;

 public:
  virtual bool Init(const MarketConfig& config) = 0;

  virtual bool Subscribe(const std::vector<std::string>& subscription_list) = 0;

  TickRB* GetTickRB() { return &rb_; }

 protected:
  void OnTick(const TickData& tick) { rb_.PutWithBlocking(tick); }

 private:
  TickRB rb_;
};

}  // namespace ft

#endif  // FT_INCLUDE_FT_MARKET_MD_GATEWAY_H_
