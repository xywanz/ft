// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "Indicator/MA.h"

namespace ft {

void MA::on_init(const TickDatabase* db) {
  uint64_t i = 0;
  const TickData* md;

  // 计算当日历史行情
  while ((md = db->get_tick(i)) != nullptr) {
    ++i;
    on_tick(db);
  }
}

void MA::on_tick(const TickDatabase* db) {
  
}

}  // namespace ft
