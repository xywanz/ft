// Copyright [2020-present] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "trader/gateway/backtest/data_feed/data_feed.h"

#include "ft/base/log.h"
#include "trader/gateway/backtest/data_feed/csv_data_feed.h"

namespace ft {

using CreateDataFeedFn = std::shared_ptr<DataFeed> (*)();
using DataFeedCtorMap = std::map<std::string, CreateDataFeedFn>;

DataFeedCtorMap& __GetDataFeedCtorMap() {
  static DataFeedCtorMap map;
  return map;
}

std::shared_ptr<DataFeed> CreateDataFeed(const std::string& name) {
  auto& map = __GetDataFeedCtorMap();
  auto it = map.find(name);
  if (it == map.end()) {
    return nullptr;
  } else {
    return (*it->second)();
  }
}

#define REGISTER_DATA_FEED(name, type)                              \
  static std::shared_ptr<::ft::DataFeed> __CreateDataFeed##type() { \
    return std::make_shared<type>();                                \
  }                                                                 \
  static const bool __is_##type##_registered [[gnu::unused]] = [] { \
    ::ft::__GetDataFeedCtorMap()[name] = &__CreateDataFeed##type;   \
    return true;                                                    \
  }();

REGISTER_DATA_FEED("ft.data_feed.csv", CsvDataFeed);

}  // namespace ft
