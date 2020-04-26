// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include <atomic>
#include <iostream>

#include <getopt.hpp>
#include <spdlog/spdlog.h>

#include "Api/Ctp/CtpApi.h"
#include "EventEngine.h"
#include "TestCommon.h"
#include "TradingManagement/ContractTable.h"

class DataCollector {
 public:
  DataCollector()
    : engine_(new ft::EventEngine) {
    api_.reset(new ft::CtpApi(engine_.get()));

    engine_->set_handler(ft::EV_TICK, MEM_HANDLER(DataCollector::on_tick));
  }

  bool login(const ft::LoginParams& params) {
    if (!api_->login(params)) {
      spdlog::error("[DataCollector] login. Failed to login into md server");
      return false;
    }
    is_login_ = true;
    return true;
  }

  void set_output_path(const std::string& path) {
    if (path.empty())
      return;

    path_ = path;
    if (path_.back() == '/')
      path_.pop_back();
  }

  void run() {
    engine_->run();
  }

  void on_tick(cppex::Any* data) {
    auto* tick = data->cast<ft::MarketData>();
    auto iter = ofs_map_.find(tick->ticker);
    if (iter == ofs_map_.end()) {
      std::string file = fmt::format("{}/{}-{}.csv", path_, tick->ticker, tick->date);
      std::ofstream ofs(file, std::ios_base::app);
      if (!ofs) {
        spdlog::error("Failed to open file '{}'", file);
        return;
      }
      if (ofs.tellp() == 0) {
        ofs << fmt::format("sec,msec,volume,turnover,open_interest,last_price,"
                           "open,highest,lowest,pre_close,upper_limit,lower_limit,"
                           "ask1,ask2,ask3,ask4,ask5,"
                           "bid1,bid2,bid3,bid4,bid5,"
                           "ask1_vol,ask2_vol,ask3_vol,ask4_vol,ask5_vol,"
                           "bid1_vol,bid2_vol,bid3_vol,bid4_vol,bid5_vol\n");
        ofs.flush();
      }

      auto res = ofs_map_.emplace(std::move(file), std::move(ofs));
      iter = res.first;
    }

    auto& ofs = iter->second;
    auto line = fmt::format(
          "{},{},{},{},{},{},"
          "{},{},{},{},{},{},"
          "{},{},{},{},{},"
          "{},{},{},{},{},"
          "{},{},{},{},{},"
          "{},{},{},{},{}\n",
          tick->time_sec, tick->time_ms, tick->volume,
          tick->turnover, tick->open_interest, tick->last_price,
          tick->open_price, tick->highest_price, tick->lowest_price,
          tick->pre_close_price, tick->upper_limit_price, tick->lower_limit_price,
          tick->ask[0], tick->ask[1], tick->ask[2], tick->ask[3], tick->ask[4],
          tick->bid[0], tick->bid[1], tick->bid[2], tick->bid[3], tick->bid[4],
          tick->ask_volume[0], tick->ask_volume[1], tick->ask_volume[2],
          tick->ask_volume[3], tick->ask_volume[4], tick->bid_volume[0],
          tick->bid_volume[1], tick->bid_volume[2], tick->bid_volume[3],
          tick->bid_volume[4]);
    spdlog::info(line);
    ofs << line;
    ofs.flush();
  }

 private:
  std::unique_ptr<ft::EventEngine> engine_;
  std::unique_ptr<ft::GeneralApi> api_;
  std::map<std::string, ft::Contract> contracts_;
  std::atomic<bool> is_login_ = false;

  std::map<std::string, std::ofstream> ofs_map_;

  std::string path_ = "./";
};


int main() {
  std::string path = getarg("./", "--path");
  std::string login_config_file = getarg("../config/login.yaml", "--login-config");
  std::string contracts_file = getarg("../config/contracts.csv", "--contracts-file");

  ft::LoginParams params;
  if (!load_login_params(login_config_file, &params)) {
    spdlog::error("Invalid file of login config");
    exit(-1);
  }
  params.set_front_addr("");

  if (!ft::ContractTable::init(contracts_file)) {
    spdlog::error("Invalid file of contract list");
    exit(-1);
  }

  auto* collector = new DataCollector;
  if (!collector->login(params)) {
    exit(-1);
  }

  collector->set_output_path(path);
  collector->run();
}
