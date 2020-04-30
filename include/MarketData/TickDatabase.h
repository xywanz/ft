// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_INCLUDE_MARKETDATA_TICKDATABASE_H_
#define FT_INCLUDE_MARKETDATA_TICKDATABASE_H_

#include <string>
#include <vector>

#include "Base/DataStruct.h"

namespace ft {

class TickDatabase {
 public:
  explicit TickDatabase(const std::string& ticker);

  void on_tick(const TickData* data);

  const TickData* get_tick(std::size_t offset) const  {
    if (offset >= tick_data_.size())
      return nullptr;
    return &*(tick_data_.rbegin() + offset);
  }

 private:
  std::string ticker_;
  std::vector<TickData> tick_data_;
};

}  // namespace ft

#endif  // FT_INCLUDE_MARKETDATA_TICKDATABASE_H_
