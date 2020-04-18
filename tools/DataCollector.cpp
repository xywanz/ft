// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include <atomic>
#include <iostream>

#include <getopt.hpp>
#include <spdlog/spdlog.h>

#include "ctp/CtpApi.h"
#include "EventEngine.h"

const char* kSimnowMdAddr[] = {
  "tcp://180.168.146.187:10131",
  "tcp://180.168.146.187:10110",
  "tcp://180.168.146.187:10111",
  "tcp://218.202.237.33:10112"
};

const char* kBrokerID = "9999";
const char* kInvestorID = "122899";
const char* kPasswd = "lsk4129691";
const char* kAuthCode = "0000000000000000";
const char* kAppID = "simnow_client_test";

class DataCollector {
 public:
  DataCollector()
    : engine_(new ft::EventEngine) {
    api_.reset(new ft::CtpApi(engine_.get()));

    engine_->set_handler(ft::EV_TICK, MEM_HANDLER(DataCollector::on_tick));
    engine_->run(false);
  }

  bool login(const ft::LoginParams& params) {
    if (!api_->login(params)) {
      spdlog::error("[DataCollector] login. Failed to login into md server");
      return false;
    }
    is_login_ = true;
    return true;
  }

  void join() {
    api_->join();
  }

  void on_tick(cppex::Any* data) {
    auto* tick = data->cast<ft::MarketData>();
    auto iter = ofs_map_.find(tick->ticker);
    if (iter == ofs_map_.end()) {
      std::string file = fmt::format("{}-{}.csv", tick->ticker, tick->date);
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
};


int main() {
  std::size_t front_index = getarg(0UL, "--front");
  int log_level = getarg(0, "--loglevel");

  spdlog::set_level(static_cast<spdlog::level::level_enum>(log_level));
  ft::ContractTable::init("./contracts.csv");

  auto* collector = new DataCollector;
  ft::LoginParams params;

  if (front_index >= sizeof(kSimnowMdAddr) / sizeof(kSimnowMdAddr[0]))
    exit(-1);

  params.set_md_server_addr(kSimnowMdAddr[front_index]);
  params.set_broker_id(kBrokerID);
  params.set_investor_id(kInvestorID);
  params.set_passwd(kPasswd);
  params.set_auth_code(kAuthCode);
  params.set_app_id(kAppID);
  params.set_subscribed_list({"rb2009.SHFE", "zn2009.SHFE"});

  if (!collector->login(params)) {
    exit(-1);
  }

  collector->join();
}
