// Copyright [2020-2021] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_INCLUDE_FT_COMPONENT_POSITION_MANAGER_H_
#define FT_INCLUDE_FT_COMPONENT_POSITION_MANAGER_H_

#include <functional>
#include <string>
#include <unordered_map>

#include "ft/base/config.h"
#include "ft/base/trade_msg.h"
#include "ft/component/position/calculator.h"

namespace ft {

class PositionManager {
 public:
  using CallbackType = std::function<void(const std::string& strategy, const Position&)>;

  static constexpr const char* kCommonPosPool = "common";

 public:
  PositionManager();

  bool Init(const FlareTraderConfig& conf, CallbackType&& f);

  // 直接设置仓位，会将现有的仓位覆盖
  bool SetPosition(const std::string& strategy, const Position& pos);

  // 二级市场买卖申报，更新交易品种的待成交数量
  bool UpdatePending(const std::string& strategy, uint32_t ticker_id, Direction direction,
                     Offset offset, int changed);

  // 二级市场买卖成交，更新交易品种的持仓
  bool UpdateTraded(const std::string& strategy, uint32_t ticker_id, Direction direction,
                    Offset offset, int traded, double traded_price);

  // 将仓位从src策略转移到dst策略
  bool MovePosition(const std::string& src_strategy, const std::string& dst_strategy,
                    uint32_t ticker, Direction direction, int volume);

  // 更新浮动盈亏
  bool UpdateFloatPnl(const std::string& strategy, uint32_t ticker_id, double bid, double ask);

  const Position* GetPosition(const std::string& strategy, uint32_t ticker_id) const;

 private:
  PositionCalculator* FindCalculator(const std::string& strategy) {
    auto it = st_pos_calculators_.find(strategy);
    if (it == st_pos_calculators_.end()) {
      return &st_pos_calculators_[kCommonPosPool];
    }
    return &it->second;
  }

 private:
  std::unordered_map<std::string, PositionCalculator> st_pos_calculators_;
  CallbackType cb_;
};

}  // namespace ft

#endif  // FT_INCLUDE_FT_COMPONENT_POSITION_MANAGER_H_
