// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_INCLUDE_MARKETDATA_TICKDB_H_
#define FT_INCLUDE_MARKETDATA_TICKDB_H_

#include <string>
#include <vector>

#include "Base/DataStruct.h"

namespace ft {

class TickDB {
 public:
  explicit TickDB(const std::string& ticker);

  void process_tick(const TickData* data);

  const TickData* get_tick(std::size_t offset = 0) const {
    if (offset >= tick_data_.size()) return nullptr;
    return &*(tick_data_.rbegin() + offset);
  }

  std::size_t get_tick_count() const { return tick_data_.size(); }

 private:
  std::string ticker_;
  std::vector<TickData> tick_data_;
};

}  // namespace ft

#endif  // FT_INCLUDE_MARKETDATA_TICKDB_H_
