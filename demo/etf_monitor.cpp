// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include <vector>

#include "ft/strategy/strategy.h"
#include "ft/utils/misc.h"
#include "spdlog/spdlog.h"
#include "trading_server/risk_management/etf/etf_table.h"

using namespace ft;

struct Snapshot {
  double last_price;
  double iopv;
};

class EtfMonitor : public Strategy {
 public:
  void OnInit() override {
    spdlog::info("[GridStrategy::on_init]");

    EtfTable::Init("../config/etf_list.csv", "../config/etf_components.csv");

    auto etf = EtfTable::get_by_ticker("159901");
    if (!etf) abort();

    std::vector<std::string> sub_list;
    sub_list.emplace_back("159901");
    for (const auto& [ticker_id, component] : etf->components) {
      UNUSED(ticker_id);
      sub_list.emplace_back(component.contract->ticker);
    }
    Subscribe(sub_list);

    etf_ = etf;
    components_ = &etf->components;
    etf_contract_ = ContractTable::get_by_ticker("159901");
  }

  void OnTick(const ft::TickData& tick) override {
    auto contract = ContractTable::get_by_index(tick.ticker_id);
    assert(contract);

    auto& s = snapshots_[tick.ticker_id];
    s.last_price = tick.last_price;
    if (tick.ticker_id == etf_contract_->ticker_id) s.iopv = tick.etf.iopv;

    if (snapshots_.size() == components_->size() + 1) {
      double value = etf_->cash_component + etf_->must_cash_substitution;
      for (const auto& [ticker_id, snapshot] : snapshots_) {
        if (ticker_id == etf_contract_->ticker_id) continue;

        auto& component = components_->find(ticker_id)->second;
        value += component.volume * snapshot.last_price;
      }

      double value_L1 = value / etf_->unit;
      spdlog::info("L1:{:.4f}  L2:{:.4f}", value_L1,
                   snapshots_[etf_contract_->ticker_id].last_price);
    }
  }

 private:
  const ETF* etf_;
  const std::unordered_map<uint32_t, ComponentStock>* components_;
  std::unordered_map<uint32_t, Snapshot> snapshots_;
  const Contract* etf_contract_;
};

EXPORT_STRATEGY(EtfMonitor);
