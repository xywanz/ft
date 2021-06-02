// Copyright [2020-2021] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "ft/component/position/manager.h"

#include "ft/base/log.h"
#include "ft/utils/protocol_utils.h"

namespace ft {

PositionManager::PositionManager() {}

bool PositionManager::Init(const FlareTraderConfig& conf, CallbackType&& f) {
  for (auto& strategy_conf : conf.strategy_config_list) {
    st_pos_calculators_.emplace(strategy_conf.strategy_name, PositionCalculator{});
  }
  st_pos_calculators_.emplace(kCommonPosPool, PositionCalculator{});

  cb_ = std::move(f);
  if (cb_) {
    for (auto& pair : st_pos_calculators_) {
      auto& strategy = pair.first;
      pair.second.SetCallback([this, strategy](const Position& pos) { cb_(strategy, pos); });
    }
  }
  return true;
}

bool PositionManager::SetPosition(const std::string& strategy, const Position& pos) {
  auto* calculator = FindCalculator(strategy);
  if (!calculator || !calculator->SetPosition(pos)) {
    LOG_ERROR("[PositionManager::SetPosition] strategy:{}", strategy);
    return false;
  }
  return true;
}

bool PositionManager::UpdatePending(const std::string& strategy, uint32_t ticker_id,
                                    Direction direction, Offset offset, int changed) {
  auto* calculator = FindCalculator(strategy);
  if (!calculator || !calculator->UpdatePending(ticker_id, direction, offset, changed)) {
    LOG_ERROR("[PositionManager::UpdatePending] strategy:{}, ticker_id:{}, {}{}, changed:{}",
              strategy, ticker_id, ToString(direction), ToString(offset), changed);
    return false;
  }
  return true;
}

bool PositionManager::UpdateTraded(const std::string& strategy, uint32_t ticker_id,
                                   Direction direction, Offset offset, int traded,
                                   double traded_price) {
  auto* calculator = FindCalculator(strategy);
  if (!calculator ||
      !calculator->UpdateTraded(ticker_id, direction, offset, traded, traded_price)) {
    LOG_ERROR(
        "[PositionManager::UpdateTraded] strategy:{}, ticker_id:{}, {}{}, traded:{}, "
        "traded_price:{}",
        strategy, ticker_id, ToString(direction), ToString(offset), traded, traded_price);
    return false;
  }
  return true;
}

bool PositionManager::MovePosition(const std::string& src_strategy, const std::string& dst_strategy,
                                   uint32_t ticker_id, Direction direction, int volume) {
  if (volume < 0) {
    LOG_ERROR("invalid volume moved: {}", volume);
    return false;
  }

  auto src_it = st_pos_calculators_.find(src_strategy);
  auto dst_it = st_pos_calculators_.find(dst_strategy);
  if (src_it == st_pos_calculators_.end() || dst_it == st_pos_calculators_.end()) {
    LOG_ERROR("PositionManager::MovePosition. strategy not found. {} {} {} {} {}", src_strategy,
              dst_strategy, ticker_id, ToString(direction), volume);
    return false;
  }

  auto& src_pos_calc = src_it->second;
  auto& dst_pos_calc = dst_it->second;
  if (!src_pos_calc.ModifyPostition(ticker_id, direction, -volume)) {
    LOG_ERROR("PositionManager::MovePosition. position not enough. {} {} {} {} {}", src_strategy,
              dst_strategy, ticker_id, ToString(direction), volume);
    return false;
  }
  dst_pos_calc.ModifyPostition(ticker_id, direction, volume);
  return true;
}

bool PositionManager::UpdateFloatPnl(const std::string& strategy, uint32_t ticker_id, double bid,
                                     double ask) {
  auto it = st_pos_calculators_.find(strategy);
  if (it == st_pos_calculators_.end() || !it->second.UpdateFloatPnl(ticker_id, bid, ask)) {
    LOG_ERROR("[PositionManager::UpdateFloatPnl] failed");
    return false;
  }
  return true;
}

const Position* PositionManager::GetPosition(const std::string& strategy,
                                             uint32_t ticker_id) const {
  auto st_iter = st_pos_calculators_.find(strategy);
  if (st_iter == st_pos_calculators_.end()) {
    LOG_ERROR("[PositionManager::GetPosition] strategy not found: {}", strategy);
    return nullptr;
  }

  auto& calculator = st_iter->second;
  return calculator.GetPosition(ticker_id);
}

}  // namespace ft
