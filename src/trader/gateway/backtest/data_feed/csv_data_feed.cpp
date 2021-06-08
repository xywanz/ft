// Copyright [2020-present] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "trader/gateway/backtest/data_feed/csv_data_feed.h"

#include <fstream>
#include <functional>
#include <map>
#include <utility>
#include <vector>

#include "ft/base/contract_table.h"
#include "ft/base/log.h"
#include "ft/utils/string_utils.h"

namespace ft {

class CsvReader {
 public:
  explicit CsvReader(const std::string& file) : ifs_(file) {
    if (ifs_) {
      std::string head_line;
      if (!std::getline(ifs_, head_line)) {
        ifs_.close();
        return;
      }
      std::vector<std::string> header;
      StringSplit(head_line, ",", &header, false);
      if (header.empty()) {
        ifs_.close();
        return;
      }
      for (int i = 0; i < static_cast<int>(header.size()); ++i) {
        if (header[i].empty()) {
          ifs_.close();
          return;
        }
        if (name_to_index_.find(header[i]) != name_to_index_.end()) {
          ifs_.close();
          return;
        }
        name_to_index_.emplace(header[i], i);
      }
    }
  }

  bool NextLine() {
    if (!ifs_) {
      return false;
    }

    std::string line;
    if (!std::getline(ifs_, line)) {
      return false;
    }
    std::vector<std::string> fields;
    StringSplit(line, ",", &fields, false);
    if (fields.size() != name_to_index_.size()) {
      throw std::runtime_error("invalid csv file");
    }
    fields_ = std::move(fields);
    return true;
  }

  std::string GetFieldString(const std::string& field_name) const {
    return GetFieldRef(field_name);
  }

  double GetFieldDouble(const std::string& field_name) const {
    return std::stod(GetFieldRef(field_name));
  }

  int GetFieldInt(const std::string& field_name) const {
    return std::stoi(GetFieldRef(field_name));
  }

  uint64_t GetFieldULong(const std::string& field_name) const {
    return std::stoul(GetFieldRef(field_name));
  }

  operator bool() const { return !!ifs_; }

 private:
  const std::string& GetFieldRef(const std::string& field_name) const {
    auto it = name_to_index_.find(field_name);
    if (it == name_to_index_.end()) {
      throw std::runtime_error("field not found");
    }
    if (fields_.empty()) {
      throw std::runtime_error("no data");
    }
    return fields_[it->second];
  }

 private:
  std::ifstream ifs_;
  std::map<std::string, int> name_to_index_;
  std::vector<std::string> fields_;
};

CsvDataFeed::~CsvDataFeed() {}

bool CsvDataFeed::Init(const std::map<std::string, std::string>& args) {
  auto data_file_iter = args.find("data_file");
  if (data_file_iter == args.end()) {
    LOG_ERROR("data_file conf not found");
    return false;
  }

  auto data_file = data_file_iter->second;
  if (!LoadCsv(data_file)) {
    LOG_ERROR("load data failed. data_file: {}", data_file);
    return false;
  }
  return true;
}

bool CsvDataFeed::LoadCsv(const std::string& file) {
  CsvReader reader(file);
  if (!reader) {
    LOG_ERROR("data feed file not found {}", file);
    return false;
  }

  while (reader.NextLine()) {
    history_data_.emplace_back(TickData{});
    auto& tick = history_data_.back();
    auto* contract = ContractTable::get_by_ticker(reader.GetFieldString("InstrumentID"));
    if (!contract) {
      LOG_ERROR("ticker {} not found", reader.GetFieldString("InstrumentID"));
      return false;
    }
    tick.ticker_id = contract->ticker_id;

    tick.local_timestamp_us = reader.GetFieldULong("LocalTimeStamp");
    tick.exchange_timestamp_us = reader.GetFieldULong("ExchangeTimeStamp");
    tick.last_price = reader.GetFieldDouble("LastPrice");
    tick.volume = reader.GetFieldULong("Volume");
    tick.turnover = reader.GetFieldDouble("Turnover");
    tick.ask[0] = reader.GetFieldDouble("AskPrice1");
    tick.ask[1] = reader.GetFieldDouble("AskPrice2");
    tick.ask[2] = reader.GetFieldDouble("AskPrice3");
    tick.ask[3] = reader.GetFieldDouble("AskPrice4");
    tick.ask[4] = reader.GetFieldDouble("AskPrice5");
    tick.ask_volume[0] = reader.GetFieldInt("AskVolume1");
    tick.ask_volume[1] = reader.GetFieldInt("AskVolume2");
    tick.ask_volume[2] = reader.GetFieldInt("AskVolume3");
    tick.ask_volume[3] = reader.GetFieldInt("AskVolume4");
    tick.ask_volume[4] = reader.GetFieldInt("AskVolume5");
    tick.bid[0] = reader.GetFieldDouble("BidPrice1");
    tick.bid[1] = reader.GetFieldDouble("BidPrice2");
    tick.bid[2] = reader.GetFieldDouble("BidPrice3");
    tick.bid[3] = reader.GetFieldDouble("BidPrice4");
    tick.bid[4] = reader.GetFieldDouble("BidPrice5");
    tick.bid_volume[0] = reader.GetFieldInt("BidVolume1");
    tick.bid_volume[1] = reader.GetFieldInt("BidVolume2");
    tick.bid_volume[2] = reader.GetFieldInt("BidVolume3");
    tick.bid_volume[3] = reader.GetFieldInt("BidVolume4");
    tick.bid_volume[4] = reader.GetFieldInt("BidVolume5");
  }

  return true;
}

bool CsvDataFeed::Feed() {
  if (data_cursor_ < history_data_.size()) {
    listener()->OnDataFeed(&history_data_[data_cursor_]);
    ++data_cursor_;
    return true;
  }
  return false;
}

}  // namespace ft
