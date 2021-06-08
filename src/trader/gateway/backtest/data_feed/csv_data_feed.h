// Copyright [2020-present] <Copyright Kevin, kevin.lau.gd@gmail.com>

#pragma once

#include <map>
#include <string>
#include <vector>

#include "trader/gateway/backtest/data_feed/data_feed.h"

namespace ft {

class CsvDataFeed : public DataFeed {
 public:
  ~CsvDataFeed();

  bool Init(const std::map<std::string, std::string>& args) override;

  bool Feed() override;

 private:
  bool LoadCsv(const std::string& file);

 private:
  std::vector<TickData> history_data_;
  uint64_t data_cursor_ = 0;
};

}  // namespace ft
