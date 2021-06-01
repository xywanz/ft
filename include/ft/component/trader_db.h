// Copyright [2020-2021] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_INCLUDE_FT_COMPONENT_TRADER_DB_H_
#define FT_INCLUDE_FT_COMPONENT_TRADER_DB_H_

#include <string>
#include <vector>

#include "ft/base/trade_msg.h"

namespace ft {

class TraderDB {
 public:
  TraderDB();

  ~TraderDB();

  bool Init(const std::string& address, const std::string& username, const std::string& password);

  bool GetPosition(const std::string& strategy, const std::string& ticker, Position* res) const;

  bool GetAllPositions(const std::string& strategy, std::vector<Position>* res) const;

  bool SetPosition(const std::string& strategy, const std::string& ticker, const Position& pos);

  bool ClearPositions(const std::string& strategy);

 private:
  void* db_impl_ = nullptr;
};

}  // namespace ft

#endif  // FT_INCLUDE_FT_COMPONENT_TRADER_DB_H_
