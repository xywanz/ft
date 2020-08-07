// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include <spdlog/spdlog.h>

#include <vector>

#include "cep/rms/etf/etf_table.h"
#include "strategy/strategy.h"
#include "utils/misc.h"

using namespace ft;

struct Snapshot {
  double last_price;
  double iopv;
};

class EtfMonitor : public Strategy {
 public:
  void on_init() override {
    spdlog::info("[GridStrategy::on_init]");

    EtfTable::init("../config/etf_list.csv", "../config/etf_components.csv");

    auto etf = EtfTable::get_by_ticker("159901");
    if (!etf) abort();

    std::vector<std::string> sub_list;
    sub_list.emplace_back("159901");
    for (const auto& [ticker_index, component] : etf->components) {
      UNUSED(ticker_index);
      sub_list.emplace_back(component.contract->ticker);
    }
    subscribe(sub_list);

    etf_ = etf;
    components_ = &etf->components;
    etf_contract_ = ContractTable::get_by_ticker("159901");
  }

  void on_tick(const ft::TickData& tick) override {
    auto contract = ContractTable::get_by_index(tick.ticker_index);
    assert(contract);

    auto& s = snapshots_[tick.ticker_index];
    s.last_price = tick.last_price;
    if (tick.ticker_index == etf_contract_->index) s.iopv = tick.etf.iopv;

    if (snapshots_.size() == components_->size() + 1) {
      double value = etf_->cash_component + etf_->must_cash_substitution;
      for (const auto& [ticker_index, snapshot] : snapshots_) {
        if (ticker_index == etf_contract_->index) continue;

        auto& component = components_->find(ticker_index)->second;
        value += component.volume * snapshot.last_price;
      }

      double value_L1 = value / etf_->unit;
      spdlog::info("L1:{:.4f}  L2:{:.4f}", value_L1,
                   snapshots_[etf_contract_->index].last_price);
    }
  }

 private:
  const ETF* etf_;
  const std::unordered_map<uint32_t, ComponentStock>* components_;
  std::unordered_map<uint32_t, Snapshot> snapshots_;
  const Contract* etf_contract_;
};

EXPORT_STRATEGY(EtfMonitor);
